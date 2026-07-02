#pragma once

#include "core/event_center.hpp"
#include "structures/math/spk_vector3.hpp"

#include <memory>

namespace pg
{
	struct MapDefinition;
	class VoxelWorld;

	struct WorldContext
	{
		int seed = 0;
		std::unique_ptr<VoxelWorld> world;
		const MapDefinition *activeMap = nullptr;

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
