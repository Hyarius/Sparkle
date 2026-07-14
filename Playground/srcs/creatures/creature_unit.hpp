#pragma once

#include "core/creature_instance_id.hpp"
#include "creatures/creature_state_derivation.hpp"
#include "feats/feat_board_progress.hpp"

#include <span>
#include <string>

namespace pg
{
	class Registries;

	// The creature the player owns: what it is, and what it has done. Nothing else.
	//
	// Deliberately absent, because each would be a second source of truth the save could contradict:
	// level, XP, AI, battle side, a current form of its own, current HP/AP/MP, a board position, a
	// status list, a shield, turn-bar fill, taming progress. Step 06 projects a session-local
	// BattleUnit from this value; the battle never writes back into it except through a result.
	struct CreatureUnit
	{
		CreatureInstanceId id;
		std::string speciesId;
		FeatBoardProgress featProgress;

		// A rebuildable convenience cache, never authoritative save data: step 18 omits it and
		// rebuildDerivedState() restores it exactly.
		DerivedCreatureState derived;

		[[nodiscard]] bool operator==(const CreatureUnit &p_other) const = default;
	};

	// Resolves the species and its board, builds the progress, derives the state, and returns one
	// complete value - or throws, leaving no half-built creature behind. p_completedNodeIds is
	// empty for a fresh creature and holds the authored preset of an encounter opponent.
	[[nodiscard]] CreatureUnit makeCreatureUnit(
		CreatureInstanceId p_id,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const DerivationContext &p_context);

	[[nodiscard]] CreatureUnit makeCreatureUnit(
		CreatureInstanceId p_id,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const Registries &p_registries);

	// Recomputes the cache from the persistent half. A save that dropped it, or a definition patch
	// that changed a reward, both go through this and through nothing else.
	void rebuildDerivedState(CreatureUnit &p_unit, const DerivationContext &p_context);
	void rebuildDerivedState(CreatureUnit &p_unit, const Registries &p_registries);
}
