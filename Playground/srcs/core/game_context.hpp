#pragma once

#include "core/event_center.hpp"

#include <memory>

namespace pg
{
	class VoxelWorld;
	class WorldNavigation;

	struct WorldContext
	{
		// GameSceneWidget publishes these only after scene construction succeeds. Scene
		// teardown releases navigation first and then the world, because the map borrows
		// the scene's GameEngine while unregistering its loaded chunk entities.
		std::unique_ptr<VoxelWorld> world;
		std::unique_ptr<WorldNavigation> navigation;
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

		GameContext();
		~GameContext();
	};
}
