#pragma once

#include "abilities/ability.hpp"
#include "ai/ai_behaviour.hpp"
#include "animation/model_definition.hpp"
#include "core/game_rules.hpp"
#include "core/registry.hpp"
#include "creatures/creature_species.hpp"
#include "encounters/biome.hpp"
#include "encounters/encounter_table.hpp"
#include "feats/feat_registry.hpp"
#include "statuses/status.hpp"
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
		Registry<Status> _statuses;
		Registry<ModelDefinition> _models;
		FeatRegistry _featBoards;
		Registry<AIBehaviour> _ai;
		Registry<CreatureSpecies> _creatures;
		Registry<EncounterTable> _encounterTables;
		Registry<BiomeDefinition> _biomes;
		Registry<PrefabDefinition> _prefabs;
		Registry<MapDefinition> _maps;

	public:
		void loadAll(const std::filesystem::path &p_dataDirectory);

		[[nodiscard]] const GameRules &gameRules() const noexcept;
		[[nodiscard]] const VoxelRegistry &voxels() const noexcept;
		[[nodiscard]] const Registry<Ability> &abilities() const noexcept;
		[[nodiscard]] const Registry<Status> &statuses() const noexcept;
		[[nodiscard]] const Registry<ModelDefinition> &models() const noexcept;
		[[nodiscard]] const FeatRegistry &featBoards() const noexcept;
		[[nodiscard]] const Registry<AIBehaviour> &ai() const noexcept;
		[[nodiscard]] const Registry<CreatureSpecies> &creatures() const noexcept;
		[[nodiscard]] const Registry<EncounterTable> &encounterTables() const noexcept;
		[[nodiscard]] const Registry<BiomeDefinition> &biomes() const noexcept;
		[[nodiscard]] const Registry<PrefabDefinition> &prefabs() const noexcept;
		[[nodiscard]] const Registry<MapDefinition> &maps() const noexcept;
	};
}
