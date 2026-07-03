#pragma once

#include "abilities/ability.hpp"
#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "encounters/biome.hpp"
#include "encounters/encounter_table.hpp"
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
		Registry<Ability> _abilities;
		Registry<EncounterTable> _encounterTables;
		Registry<BiomeDefinition> _biomes;
		Registry<PrefabDefinition> _prefabs;
		Registry<MapDefinition> _maps;

	public:
		void loadAll(const std::filesystem::path &p_dataDirectory);

		[[nodiscard]] const GameRules &gameRules() const noexcept;
		[[nodiscard]] const VoxelRegistry &voxels() const noexcept;
		[[nodiscard]] const Registry<Ability> &abilities() const noexcept;
		[[nodiscard]] const Registry<EncounterTable> &encounterTables() const noexcept;
		[[nodiscard]] const Registry<BiomeDefinition> &biomes() const noexcept;
		[[nodiscard]] const Registry<PrefabDefinition> &prefabs() const noexcept;
		[[nodiscard]] const Registry<MapDefinition> &maps() const noexcept;
	};
}
