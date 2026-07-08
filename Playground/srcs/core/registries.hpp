#pragma once

#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/biome_definition.hpp"
#include "world/generator/world_plan.hpp"
#include "world/prefab_definition.hpp"

#include <filesystem>

namespace pg
{
	class Registries
	{
	private:
		GameRules _gameRules;
		VoxelRegistry _voxels;
		Registry<BiomeDefinition> _biomes;
		Registry<PrefabDefinition> _prefabs;
		PlanPlacementRules _placementRules;

	public:
		void loadAll(const std::filesystem::path &p_dataDirectory);

		[[nodiscard]] const GameRules &gameRules() const noexcept;
		[[nodiscard]] const VoxelRegistry &voxels() const noexcept;
		[[nodiscard]] const Registry<BiomeDefinition> &biomes() const noexcept;
		[[nodiscard]] const Registry<PrefabDefinition> &prefabs() const noexcept;
		// Which prefab worldgen inserts for stairways and per entity kind
		// (resources/data/worldgen/placements.json).
		[[nodiscard]] const PlanPlacementRules &placementRules() const noexcept;
	};
}
