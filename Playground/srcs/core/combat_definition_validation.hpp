#pragma once

#include "abilities/ability_definition.hpp"
#include "battle_objects/battle_object_definition.hpp"
#include "core/registry.hpp"
#include "statuses/status_definition.hpp"

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
}
