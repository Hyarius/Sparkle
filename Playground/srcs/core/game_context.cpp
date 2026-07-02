#include "core/game_context.hpp"

#include "world/voxel_world.hpp"

namespace pg
{
	WorldContext::WorldContext() = default;
	WorldContext::~WorldContext() = default;
	WorldContext::WorldContext(WorldContext &&) noexcept = default;
	WorldContext &WorldContext::operator=(WorldContext &&) noexcept = default;

	GameContext::GameContext() = default;
	GameContext::~GameContext() = default;
}
