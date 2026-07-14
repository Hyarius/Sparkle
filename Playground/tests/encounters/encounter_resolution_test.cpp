#include <gtest/gtest.h>

#include "battle/battle_rng.hpp"
#include "encounters/encounter_resolver.hpp"

#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	[[nodiscard]] pg::EncounterSpawnDefinition spawn(std::string_view p_id)
	{
		pg::EncounterSpawnDefinition result;
		result.id = p_id;
		result.speciesId = "training-sprout";
		result.aiBehaviourId = "training-aggressive";
		return result;
	}

	[[nodiscard]] pg::EncounterTeamDefinition team(std::string_view p_id, std::uint64_t p_weight)
	{
		pg::EncounterTeamDefinition result;
		result.id = p_id;
		result.weight = p_weight;
		result.members = {spawn("first"), spawn("second")};
		return result;
	}

	// Tiers 0, 2 and 5: sparse on purpose, which is what tier fallback is for.
	[[nodiscard]] pg::EncounterDefinition sparseEncounter()
	{
		pg::EncounterDefinition result;
		result.id = "training-wild";
		result.kind = pg::EncounterKind::Wild;
		result.allowsTaming = true;
		result.repeatable = true;
		result.triggerChancePermille = 1000;
		result.board = pg::LiveWorldBoardPolicyDefinition{};
		result.tiers = {
			pg::EncounterTierDefinition{.tier = 0, .teams = {team("early", 1)}},
			pg::EncounterTierDefinition{.tier = 2, .teams = {team("middle", 1)}},
			pg::EncounterTierDefinition{.tier = 5, .teams = {team("late", 1)}}};
		return result;
	}

	[[nodiscard]] pg::EncounterDefinition weightedEncounter()
	{
		pg::EncounterDefinition result = sparseEncounter();
		result.tiers = {pg::EncounterTierDefinition{
			.tier = 0,
			.teams = {team("common", 70), team("uncommon", 25), team("rare", 5)}}};
		return result;
	}
}

TEST(EncounterResolutionTest, FallsBackToTheGreatestAuthoredTierAtOrBelowTheRequest)
{
	const pg::EncounterDefinition encounter = sparseEncounter();

	EXPECT_EQ(pg::selectEncounterTier(encounter, 0).tier, 0U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 1).tier, 0U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 2).tier, 2U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 3).tier, 2U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 4).tier, 2U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 5).tier, 5U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 8).tier, 5U);
	EXPECT_EQ(pg::selectEncounterTier(encounter, 9).tier, 5U) << "tier 9 is reserved, but it is still a legal request";

	// Above the maximum is a caller bug: rejected, never clamped and never wrapped.
	EXPECT_THROW(auto value = pg::selectEncounterTier(encounter, 10), std::invalid_argument);
	// And it consumes nothing on the way out.
	pg::BattleRng rng(7);
	EXPECT_THROW(auto value = pg::resolveEncounter(encounter, 10, rng), std::invalid_argument);
	EXPECT_EQ(rng.drawCount(), 0U) << "an invalid request leaves the stream exactly where it found it";
}

TEST(EncounterResolutionTest, WalksTheWeightsInAuthoredOrderWithOneBoundedDraw)
{
	const pg::EncounterDefinition encounter = weightedEncounter();
	const pg::EncounterTierDefinition &tier = encounter.tiers[0];

	// One uniformBelow(total) call, whatever the outcome.
	pg::BattleRng rng(1);
	(void)pg::selectWeightedTeam(tier, rng);
	const std::size_t afterOne = rng.drawCount();
	EXPECT_GE(afterOne, 1U);
	(void)pg::selectWeightedTeam(tier, rng);
	EXPECT_GT(rng.drawCount(), afterOne);

	// The exact boundaries: ticket 0..69 is common, 70..94 uncommon, 95..99 rare. Sampling many
	// seeds has to produce every team and never anything else.
	std::set<std::string> seen;
	for (std::uint64_t seed = 0; seed < 200; ++seed)
	{
		pg::BattleRng local(seed);
		const std::string chosen = pg::selectWeightedTeam(tier, local).id;
		ASSERT_TRUE(chosen == "common" || chosen == "uncommon" || chosen == "rare") << chosen;
		seen.insert(chosen);
	}
	EXPECT_EQ(seen.size(), 3U) << "every authored team is reachable";

	// Same seed, same answer - the only reason a battle is replayable from its seed.
	for (std::uint64_t seed = 0; seed < 20; ++seed)
	{
		pg::BattleRng first(seed);
		pg::BattleRng second(seed);
		EXPECT_EQ(pg::selectWeightedTeam(tier, first).id, pg::selectWeightedTeam(tier, second).id);
		EXPECT_EQ(first.drawCount(), second.drawCount());
	}
}

TEST(EncounterResolutionTest, RollsTheTriggerChanceExactlyOnceWhateverThePermille)
{
	pg::EncounterDefinition always = sparseEncounter();
	always.triggerChancePermille = 1000;
	pg::EncounterDefinition never = sparseEncounter();
	never.triggerChancePermille = 0;
	pg::EncounterDefinition almostNever = sparseEncounter();
	almostNever.triggerChancePermille = 1;
	pg::EncounterDefinition almostAlways = sparseEncounter();
	almostAlways.triggerChancePermille = 999;

	for (std::uint64_t seed = 0; seed < 50; ++seed)
	{
		pg::BattleRng certain(seed);
		EXPECT_TRUE(pg::rollEncounterTrigger(always, certain)) << "a permille of 1000 always triggers";
		// Even at 0 and 1000 the call is made: a Bush step has to cost the same number of bounded
		// samples whatever the biome authored, or the stream would fork on the content.
		EXPECT_GE(certain.drawCount(), 1U);

		pg::BattleRng impossible(seed);
		EXPECT_FALSE(pg::rollEncounterTrigger(never, impossible));
		EXPECT_GE(impossible.drawCount(), 1U);
		EXPECT_EQ(impossible.drawCount(), certain.drawCount()) << "and it costs the same draws either way";
	}

	// The rejection sampler behind uniformBelow may consume more than one raw draw, and drawCount()
	// reports the real, seed-dependent number rather than a nominal one.
	std::size_t total = 0;
	for (std::uint64_t seed = 0; seed < 100; ++seed)
	{
		pg::BattleRng rng(seed);
		(void)pg::rollEncounterTrigger(almostNever, rng);
		EXPECT_GE(rng.drawCount(), 1U);
		total += rng.drawCount();
	}
	EXPECT_GE(total, 100U);

	// A 999 permille chance is not a certainty: some seed has to say no.
	bool refusedOnce = false;
	for (std::uint64_t seed = 0; seed < 5000 && !refusedOnce; ++seed)
	{
		pg::BattleRng rng(seed);
		refusedOnce = !pg::rollEncounterTrigger(almostAlways, rng);
	}
	EXPECT_TRUE(refusedOnce);
}

TEST(EncounterResolutionTest, ResolvesToACopiedRecipeAndNothingMore)
{
	const pg::EncounterDefinition encounter = weightedEncounter();

	pg::BattleRng rng(42);
	const pg::ResolvedEncounter resolved = pg::resolveEncounter(encounter, 3, rng);

	EXPECT_EQ(resolved.encounterDefinitionId, "training-wild");
	EXPECT_EQ(resolved.kind, pg::EncounterKind::Wild);
	EXPECT_TRUE(resolved.allowsTaming);
	EXPECT_TRUE(resolved.repeatable);
	EXPECT_EQ(resolved.resolvedTier, 0U) << "the only authored tier, whatever the progression";
	// The roster is the team's member list, in authored order.
	ASSERT_EQ(resolved.enemyRoster.size(), 2U);
	EXPECT_EQ(resolved.enemyRoster[0].id, "first");
	EXPECT_EQ(resolved.enemyRoster[1].id, "second");
	EXPECT_TRUE(std::holds_alternative<pg::LiveWorldBoardPolicyDefinition>(resolved.board));

	// One team draw and no chance roll: a named request never asks whether it happens.
	EXPECT_GE(rng.drawCount(), 1U);

	// Same seed, same input, identical result.
	pg::BattleRng replay(42);
	EXPECT_EQ(pg::resolveEncounter(encounter, 3, replay), resolved);
	EXPECT_EQ(replay.drawCount(), rng.drawCount());

	// A different seed may pick a different team, but never a different tier or board.
	std::set<std::string> teams;
	for (std::uint64_t seed = 0; seed < 100; ++seed)
	{
		pg::BattleRng local(seed);
		const pg::ResolvedEncounter other = pg::resolveEncounter(encounter, 3, local);
		EXPECT_EQ(other.resolvedTier, 0U);
		EXPECT_EQ(other.board, resolved.board);
		teams.insert(other.teamId);
	}
	EXPECT_GT(teams.size(), 1U) << "the weighted draw is a draw, not a constant";
}
