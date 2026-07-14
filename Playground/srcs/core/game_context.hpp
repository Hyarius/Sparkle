#pragma once

#include "core/event_center.hpp"
#include "player/player_data.hpp"

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
		// The persistent player value, built from registries.newGame() before the scene exists. It
		// is owned here, not by a global service: if scene construction throws, ordinary unwinding
		// destroys it and nothing survives to be half-initialised.
		//
		// Its playerCell and lastHealPoint stay at the origin until the scene commits a spawn.
		PlayerData player;

		GameContext();
		~GameContext();
	};
}
