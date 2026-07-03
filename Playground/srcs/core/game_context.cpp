#include "core/game_context.hpp"

#include "encounters/encounter_emitter.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

namespace pg
{
	WorldContext::WorldContext() = default;
	WorldContext::~WorldContext() = default;
	WorldContext::WorldContext(WorldContext &&) noexcept = default;
	WorldContext &WorldContext::operator=(WorldContext &&) noexcept = default;

	GameContext::GameContext() = default;
	GameContext::~GameContext() = default;
}
