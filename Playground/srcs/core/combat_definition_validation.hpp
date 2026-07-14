#pragma once

#include "abilities/ability_definition.hpp"
#include "ai/ai_definition.hpp"
#include "battle_objects/battle_object_definition.hpp"
#include "conditions/condition_definition.hpp"
#include "core/registry.hpp"
#include "creatures/creature_state_derivation.hpp"
#include "encounters/encounter_definition.hpp"
#include "feats/feat_board_definition.hpp"
#include "statuses/status_definition.hpp"
#include "world/biome_definition.hpp"

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

	// The same phase for an AI behaviour: its ability and status references resolve against the
	// combat registries, and its status tags have to occur on at least one status in the graph
	// being published - a misspelled tag is silently false at runtime forever, so it is caught here
	// instead. A tag on an ordinary status filter stays unresolved on purpose (later content may
	// introduce it); an AI tag is a query about the content that exists now.
	void validateAIGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<AIBehaviourDefinition> &p_behaviours);

	// Everything a species could not prove alone. p_context carries the game rules and the status,
	// ability, Feat Board and species registries of the load being validated - locals during a load
	// transaction, the published ones afterwards.
	//
	// It resolves the default abilities and passives, the Feat Board, the board's fromForm gates and
	// changeForm rewards against this species' forms, and the inline taming profile's filters; it
	// checks that no authorable set of completions can exceed the loadout slots or push an attribute
	// out of its bounds; and it derives the fresh creature to prove the baseline itself is legal.
	void validateSpeciesGraph(const DerivationContext &p_context);

	// An encounter's spawns: species, AI and completed-node preset all resolve, the preset is a
	// legal board state, and every ability the member's AI would cast is in the loadout that member
	// actually derives - an enemy cannot be authored to cast a move it does not know.
	void validateEncounterGraph(
		const Registry<EncounterDefinition> &p_encounters,
		const Registry<AIBehaviourDefinition> &p_behaviours,
		const DerivationContext &p_context);

	// Every biome the world generator can place needs somewhere for its Bushes to lead: a wild,
	// repeatable encounter table. An interior-only biome has no worldgen block and therefore no
	// link, and that is the whole rule - no encounter data is ever added to a voxel.
	void validateBiomeEncounterLinks(
		const Registry<BiomeDefinition> &p_biomes,
		const Registry<EncounterDefinition> &p_encounters);
}
