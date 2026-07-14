#pragma once

#include "abilities/ability_definition.hpp"
#include "battle_objects/battle_object_definition.hpp"
#include "conditions/condition_definition.hpp"
#include "core/registry.hpp"
#include "feats/feat_board_definition.hpp"
#include "statuses/status_definition.hpp"

#include <string>
#include <vector>

namespace pg
{
	// Phase 2 of the combat load. Phase 1 parses each domain into locals, keeping every
	// cross-definition reference as a plain string id; only once all three registries exist is
	// the whole graph checked at once.
	//
	// That is what makes a legitimate cycle authorable without an impossible load order:
	//
	//     status A hook  -> apply status B
	//     status B hook  -> place object C
	//     object C trigger -> remove status A
	//
	// There is no topological sort and no recursive expansion here - a cycle is simply three
	// string ids that all resolve. Runtime re-entry is bounded by maxEffectChainDepth in
	// step 10 instead.
	//
	// Throws JsonError naming the owning definition, the offending effect id, and the original
	// file and JSON path the effect was authored at.
	void validateCombatDefinitionGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<BattleObjectDefinition> &p_battleObjects);

	// The same phase for the progression definitions: a condition's ability/status filters and a
	// reward's ability/status grants are plain string ids until the combat registries exist.
	//
	// Form references are deliberately not resolved here. A form belongs to the species that
	// selects the board, not to the board, so step 04 resolves those through
	// validateFeatBoardFormReferences() - from the same authored file and JSON path.
	//
	// Throws JsonError naming the owning definition, the offending condition or reward, and the
	// file and path it was authored at.
	void validateFeatBoardGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<FeatBoardDefinition> &p_featBoards);

	// Conditions authored outside a Feat Board - a species' inline taming profile - resolve
	// their references through the same check. p_owner names the definition in the diagnostic.
	void validateConditionReferences(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const std::vector<ConditionSpec> &p_conditions,
		const std::string &p_owner);
}
