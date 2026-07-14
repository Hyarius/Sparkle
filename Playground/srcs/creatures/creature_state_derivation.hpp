#pragma once

#include "abilities/ability_definition.hpp"
#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "creatures/creature_attributes.hpp"
#include "creatures/creature_species_definition.hpp"
#include "feats/feat_board_definition.hpp"
#include "feats/feat_board_progress.hpp"
#include "statuses/status_definition.hpp"

#include <string>
#include <vector>

namespace pg
{
	class Registries;

	// A battle-ready creature, entirely derived. It is a cache, never a source of truth: throw it
	// away and deriveCreatureState() rebuilds exactly the same value from the species and the
	// completed nodes. Step 18 therefore leaves it out of the save file.
	struct DerivedCreatureState
	{
		CreatureAttributes attributes;
		std::vector<std::string> abilityIds;
		std::vector<std::string> passiveStatusIds;
		std::string formId;

		[[nodiscard]] bool operator==(const DerivedCreatureState &p_other) const = default;
	};

	// The definitions a derivation reads, borrowed. Deriving happens twice: from the published
	// Registries at runtime, and from the load transaction's locals - where no Registries exists
	// yet, because publishing before an encounter or a starter has validated is exactly what the
	// transaction rule forbids.
	struct DerivationContext
	{
		const GameRules *gameRules = nullptr;
		const Registry<StatusDefinition> *statuses = nullptr;
		const Registry<AbilityDefinition> *abilities = nullptr;
		const Registry<FeatBoardDefinition> *featBoards = nullptr;
		const Registry<CreatureSpeciesDefinition> *species = nullptr;
	};

	[[nodiscard]] DerivationContext derivationContextOf(const Registries &p_registries);

	// The one deterministic replay, and the reason no step owns a second one:
	//
	//     attributes = species baseline
	//     abilities  = species defaults, unique, authored order
	//     passives   = species defaults, unique, authored order
	//     form       = species default form
	//     for every completed node, in board definition order:
	//         for every reward, in authored order:
	//             bonusStat     -> checked addition
	//             unlockAbility -> append when absent
	//             removeAbility -> erase when present
	//             unlockPassive -> append when absent
	//             changeForm    -> replace the form
	//     validate the result
	//
	// Pure, idempotent, RNG-free: no level, no roll, no encounter scaling. Step 17 re-runs it
	// after new completions rather than applying a reward incrementally to a battle unit.
	//
	// Throws JsonError naming the species and the offending board/node/reward path.
	[[nodiscard]] DerivedCreatureState deriveCreatureState(
		const CreatureSpeciesDefinition &p_species,
		const FeatBoardDefinition &p_board,
		const FeatBoardProgress &p_progress,
		const DerivationContext &p_context);

	[[nodiscard]] DerivedCreatureState deriveCreatureState(
		const CreatureSpeciesDefinition &p_species,
		const FeatBoardDefinition &p_board,
		const FeatBoardProgress &p_progress,
		const Registries &p_registries);
}
