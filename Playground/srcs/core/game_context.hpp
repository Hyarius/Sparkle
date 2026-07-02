#pragma once

#include "core/event_center.hpp"
#include "structures/math/spk_vector3.hpp"

#include <memory>

namespace pg
{
	struct MapDefinition;
	class VoxelWorld;
	class WorldNavigation;

	struct WorldContext
	{
		int seed = 0;
		std::unique_ptr<VoxelWorld> world;
		std::unique_ptr<WorldNavigation> navigation;
		const MapDefinition *activeMap = nullptr;
		bool explorationActive = false;

		WorldContext();
		~WorldContext();
		WorldContext(WorldContext &&) noexcept;
		WorldContext &operator=(WorldContext &&) noexcept;
		WorldContext(const WorldContext &) = delete;
		WorldContext &operator=(const WorldContext &) = delete;
	};

	struct PlayerData
	{
		spk::Vector3 position = spk::Vector3::Zero;
	};

	struct GameContext
	{
		EventCenter events;
		WorldContext world;
		PlayerData player;

		GameContext();
		~GameContext();
	};
}
