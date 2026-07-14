#include <gtest/gtest.h>

#include "battle/battle_rng.hpp"
#include "core/paths.hpp"
#include "core/registries.hpp"
#include "encounters/encounter_resolver.hpp"
#include "player/new_game_definition.hpp"
#include "player/player_data.hpp"

#include <string>
#include <variant>
#include <vector>

// The step-04 checkpoint, end to end and on the real resources: load everything, create the run,
// and resolve the debug encounter into a recipe. No board is materialised, no unit is constructed
// and no battle starts - those are steps 05, 06 and 12.
TEST(BattleContentIntegrationTest, LoadsTheContentCreatesTheRunAndResolvesTheDebugEncounter)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	// 1. The starter the executable prints.
	const pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	ASSERT_TRUE(player.roster.team()[0].has_value());

	const pg::CreatureUnit &starter = *player.roster.team()[0];
	EXPECT_EQ(starter.id.string(), "creature-0000000000000001");
	EXPECT_EQ(starter.speciesId, "training-sprout");
	EXPECT_EQ(starter.derived.formId, "base");
	EXPECT_EQ(starter.derived.abilityIds, (std::vector<std::string>{"training-strike"}));
	EXPECT_TRUE(starter.derived.passiveStatusIds.empty());
	EXPECT_EQ(starter.derived.attributes.maxHealth, 24);
	EXPECT_EQ(starter.derived.attributes.strength, 6);
	EXPECT_EQ(starter.derived.attributes.magicPower, 3);
	EXPECT_EQ(starter.derived.attributes.maxActionPoints, 6);
	EXPECT_EQ(starter.derived.attributes.maxMovementPoints, 4);
	EXPECT_EQ(starter.derived.attributes.stamina.ticks(), 4000);
	EXPECT_EQ(player.encounterTier(), 0U) << "no gym is cleared, so the world serves its tier-0 content";

	// 2. The named debug encounter, through the same resolver wild content goes through.
	const pg::EncounterDefinition &debug = registries.encounters().get("debug-battle");
	pg::BattleRng rng(20260714);
	const pg::ResolvedEncounter resolved = pg::resolveEncounter(debug, player.encounterTier(), rng);

	EXPECT_EQ(resolved.encounterDefinitionId, "debug-battle");
	EXPECT_EQ(resolved.kind, pg::EncounterKind::Debug);
	EXPECT_FALSE(resolved.allowsTaming);
	EXPECT_TRUE(resolved.repeatable);
	EXPECT_EQ(resolved.resolvedTier, 0U);
	EXPECT_EQ(resolved.teamId, "single-sprout");

	// It overlays the live world: no arena, and the approach comes from the movement that triggers it.
	const auto &live = std::get<pg::LiveWorldBoardPolicyDefinition>(resolved.board);
	EXPECT_EQ(live.size, (std::array<int, 2>{11, 11}));
	EXPECT_EQ(live.deploymentDepth, 2);
	EXPECT_TRUE(std::holds_alternative<pg::ByLineOpponentPlacementPolicy>(live.opponentPlacement));

	// 3. The enemy recipe, and the loadout it would derive - through the ordinary creature factory,
	//    because there is no enemy class and there never will be.
	ASSERT_EQ(resolved.enemyRoster.size(), 1U);
	const pg::EncounterSpawnDefinition &enemy = resolved.enemyRoster[0];
	EXPECT_EQ(enemy.id, "sprout-a");
	EXPECT_EQ(enemy.speciesId, "training-sprout");
	EXPECT_EQ(enemy.aiBehaviourId, "training-aggressive");
	EXPECT_TRUE(enemy.completedFeatNodeIds.empty());

	const pg::CreatureUnit projected = pg::makeCreatureUnit(
		pg::CreatureInstanceId::fromSerial(1),
		enemy.speciesId,
		enemy.completedFeatNodeIds,
		registries);
	EXPECT_EQ(projected.derived, starter.derived) << "same species, same baseline, same completions";
	for (const std::string &abilityId : pg::aiCastAbilityReferences(registries.aiBehaviours().get(enemy.aiBehaviourId)))
	{
		EXPECT_NE(
			std::ranges::find(projected.derived.abilityIds, abilityId),
			projected.derived.abilityIds.end())
			<< "the AI may only cast what the enemy derives";
	}

	// 4. The board registry exists, is transactional, and is deliberately empty.
	EXPECT_EQ(registries.battleBoards().size(), 0U);

	// 5. Nothing above wrote into a definition: derivation and resolution read, and only read.
	pg::Registries reloaded;
	ASSERT_NO_THROW(reloaded.loadAll(pg::resourceRoot() / "data"));
	EXPECT_EQ(reloaded.species().get("training-sprout").baseAttributes, registries.species().get("training-sprout").baseAttributes);
	EXPECT_EQ(reloaded.encounters().get("debug-battle").tiers, registries.encounters().get("debug-battle").tiers);
	EXPECT_EQ(reloaded.newGame(), registries.newGame());

	pg::BattleRng replay(20260714);
	EXPECT_EQ(pg::resolveEncounter(reloaded.encounters().get("debug-battle"), 0, replay), resolved)
		<< "same seed, same content, same recipe";
}
