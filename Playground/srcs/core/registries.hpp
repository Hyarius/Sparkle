#pragma once

#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/map_definition.hpp"
#include "world/prefab_definition.hpp"

#include <filesystem>

namespace pg
{
	class Registries
	{
	private:
		GameRules _gameRules;
		VoxelRegistry _voxels;
		Registry<PrefabDefinition> _prefabs;
		Registry<MapDefinition> _maps;

	public:
		void loadAll(const std::filesystem::path &p_dataDirectory);

		[[nodiscard]] const GameRules &gameRules() const noexcept;
		[[nodiscard]] const VoxelRegistry &voxels() const noexcept;
		[[nodiscard]] const Registry<PrefabDefinition> &prefabs() const noexcept;
		[[nodiscard]] const Registry<MapDefinition> &maps() const noexcept;
	};
}
