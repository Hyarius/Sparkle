#pragma once

#include "battle/battle_rng.hpp"
#include "battle/battle_types.hpp"
#include "encounters/encounter_definition.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace pg
{
	// What one encounter request produced. It is a value, not a view: the board policy and the
	// enemy roster are copied, so nothing here points into a registry that a later load transaction
	// may replace.
	//
	// Resolution stops at the recipe. It materialises no geometry, constructs no battle unit and
	// allocates no persistent creature id - steps 05, 06 and 12 do those, from this.
	struct ResolvedEncounter
	{
		std::string encounterDefinitionId;
		EncounterKind kind = EncounterKind::Wild;
		bool allowsTaming = false;
		bool repeatable = true;
		std::uint32_t resolvedTier = 0;
		std::string teamId;
		EncounterBoardPolicyDefinition board;
		std::vector<EncounterSpawnDefinition> enemyRoster;

		[[nodiscard]] bool operator==(const ResolvedEncounter &p_other) const = default;
	};

	// The greatest authored tier at or below the request, which is what makes a sparse table work:
	// an encounter authored for tiers 0 and 4 keeps serving tier 0 until the fourth gym falls.
	//
	// A request above MaximumProgressionTier is rejected, never clamped and never wrapped: it means
	// the caller has a bug, and quietly serving tier 9 would hide it. Consumes no RNG.
	[[nodiscard]] const EncounterTierDefinition &selectEncounterTier(
		const EncounterDefinition &p_definition,
		std::uint32_t p_progressionTier);

	// One uniformBelow(totalWeight) draw, then a walk down the authored order subtracting weights.
	// Definition validation guarantees a positive total, so the walk cannot fall off the end.
	[[nodiscard]] const EncounterTeamDefinition &selectWeightedTeam(
		const EncounterTierDefinition &p_tier,
		BattleRng &p_rng);

	// Exactly one uniformBelow(1000) call, even at 0 and at 1000, so a Bush step costs the same
	// number of bounded samples whatever the chance is. That one call is unbiased rejection
	// sampling, so it consumes one or more raw draws and BattleRng::drawCount() reports the real,
	// seed-dependent number.
	//
	// Only a biome-triggered wild encounter rolls it. A named or debug encounter proceeds straight
	// to the team draw.
	[[nodiscard]] bool rollEncounterTrigger(const EncounterDefinition &p_definition, BattleRng &p_rng);

	// Tier selection, then one weighted team draw. A rejected request (a tier above the maximum)
	// throws before touching the generator, so an invalid call leaves the stream exactly where it
	// found it.
	[[nodiscard]] ResolvedEncounter resolveEncounter(
		const EncounterDefinition &p_definition,
		std::uint32_t p_progressionTier,
		BattleRng &p_rng);
}
