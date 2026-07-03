#pragma once

#include "core/event_center.hpp"
#include "creatures/player_data.hpp"
#include "structures/math/spk_vector3.hpp"

#include <memory>

namespace pg
{
	struct BiomeDefinition;
	class Registries;
	class EncounterEmitter;
	struct MapDefinition;
	class VoxelWorld;
	class WorldNavigation;

	struct WorldContext
	{
		int seed = 0;
		std::unique_ptr<VoxelWorld> world;
		std::unique_ptr<WorldNavigation> navigation;
		std::unique_ptr<EncounterEmitter> encounterEmitter;
		const MapDefinition *activeMap = nullptr;
		const BiomeDefinition *activeBiome = nullptr;
		bool explorationActive = false;

		WorldContext();
		~WorldContext();
		WorldContext(WorldContext &&) noexcept;
		WorldContext &operator=(WorldContext &&) noexcept;
		WorldContext(const WorldContext &) = delete;
		WorldContext &operator=(const WorldContext &) = delete;
	};

	struct GameContext
	{
		EventCenter events;
		WorldContext world;
		PlayerData player;

		GameContext();
		~GameContext();
		void newGame(const Registries &p_registries);
	};
}
