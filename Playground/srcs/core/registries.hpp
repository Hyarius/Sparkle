#pragma once

#include "abilities/ability_definition.hpp"
#include "battle_objects/battle_object_definition.hpp"
#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "feats/feat_board_definition.hpp"
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
		// rewards are validated against the combat registries above; their form references wait
		// for the species that selects the board. Nothing evaluates or progresses them yet.
		[[nodiscard]] const Registry<FeatBoardDefinition> &featBoards() const noexcept;
	};
}
