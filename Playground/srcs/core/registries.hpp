#pragma once

#include "abilities/ability_definition.hpp"
#include "ai/ai_definition.hpp"
#include "battle_objects/battle_object_definition.hpp"
#include "board/handcrafted_battle_board_definition.hpp"
#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "creatures/creature_species_definition.hpp"
#include "encounters/encounter_definition.hpp"
#include "feats/feat_board_definition.hpp"
#include "player/new_game_definition.hpp"
#include "statuses/status_definition.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_family_definition.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/biome_definition.hpp"
#include "world/generator/town_composition.hpp"
#include "world/generator/world_plan.hpp"
#include "world/interior_definition.hpp"
#include "world/prefab_definition.hpp"

#include <filesystem>

namespace pg
{
	class Registries
	{
	private:
		GameRules _gameRules;
		ShapeCatalog _shapes;
		VoxelFamilyCatalog _voxelFamilies;
		VoxelRegistry _voxels;
		Registry<BiomeDefinition> _biomes;
		Registry<PrefabDefinition> _prefabs;
		Registry<InteriorDefinition> _interiors;
		PlanPlacementRules _placementRules;
		TownCompositionCatalog _townCompositions;
		Registry<StatusDefinition> _statuses;
		Registry<AbilityDefinition> _abilities;
		Registry<BattleObjectDefinition> _battleObjects;
		Registry<FeatBoardDefinition> _featBoards;
		Registry<AIBehaviourDefinition> _aiBehaviours;
		Registry<CreatureSpeciesDefinition> _species;
		Registry<HandcraftedBattleBoardDefinition> _battleBoards;
		Registry<EncounterDefinition> _encounters;
		NewGameDefinition _newGame;

	public:
		void loadAll(const std::filesystem::path &p_dataDirectory);

		[[nodiscard]] const GameRules &gameRules() const noexcept;
		// Data-driven voxel geometry (resources/data/shapes), the factory the voxel
		// registry instantiates its render shapes from.
		[[nodiscard]] const ShapeCatalog &shapes() const noexcept;
		[[nodiscard]] const VoxelFamilyCatalog &voxelFamilies() const noexcept;
		[[nodiscard]] const VoxelRegistry &voxels() const noexcept;
		[[nodiscard]] const Registry<BiomeDefinition> &biomes() const noexcept;
		[[nodiscard]] const Registry<PrefabDefinition> &prefabs() const noexcept;
		// Interior composition recipes referenced by prefab "interior" links
		// (resources/data/interiors).
		[[nodiscard]] const Registry<InteriorDefinition> &interiors() const noexcept;
		// Which prefab worldgen inserts for stairways and per entity kind
		// (resources/data/worldgen/placements.json).
		[[nodiscard]] const PlanPlacementRules &placementRules() const noexcept;
		[[nodiscard]] const TownCompositionCatalog &townCompositions() const noexcept;
		// The combat vocabulary (resources/data/statuses, abilities, battle-objects). The three
		// reference each other by string id and are validated as one graph once all are parsed,
		// so an authored cycle needs no load order. No runtime consumes them yet.
		[[nodiscard]] const Registry<StatusDefinition> &statuses() const noexcept;
		[[nodiscard]] const Registry<AbilityDefinition> &abilities() const noexcept;
		[[nodiscard]] const Registry<BattleObjectDefinition> &battleObjects() const noexcept;
		// The Feat Boards a species may select (resources/data/featboards). Their conditions and
		// rewards are validated against the combat registries above; their form references are
		// resolved by the species that selects the board. Nothing evaluates or progresses them yet.
		[[nodiscard]] const Registry<FeatBoardDefinition> &featBoards() const noexcept;
		// Ordered-rule enemy behaviours (resources/data/ai). Data only: step 11 evaluates them.
		[[nodiscard]] const Registry<AIBehaviourDefinition> &aiBehaviours() const noexcept;
		// The creatures the game knows (resources/data/creatures): a baseline, a Feat Board, forms,
		// and an optional taming profile. No level and no scaling curve exists anywhere.
		[[nodiscard]] const Registry<CreatureSpeciesDefinition> &species() const noexcept;
		// Handcrafted arenas (resources/data/battle-boards). Deliberately empty at this checkpoint:
		// the loader and the schema are real and tested, and step 19 authors the first Gym arena.
		[[nodiscard]] const Registry<HandcraftedBattleBoardDefinition> &battleBoards() const noexcept;
		// What the world can throw at the player (resources/data/encounter-tables). Resolution is in
		// encounters/encounter_resolver.hpp; nothing triggers or constructs a battle yet.
		[[nodiscard]] const Registry<EncounterDefinition> &encounters() const noexcept;
		// The starting state of a run (resources/data/config/new-game.json). Immutable content
		// loaded in this same transaction - not a save file, which step 18 defines separately.
		[[nodiscard]] const NewGameDefinition &newGame() const noexcept;
	};
}
