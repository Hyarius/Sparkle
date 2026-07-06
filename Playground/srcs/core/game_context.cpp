#include "core/game_context.hpp"

#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
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

	void GameContext::newGame(const Registries &p_registries)
	{
		player = PlayerData{};
		clearedGyms.clear();
		clearedTrainers.clear();
		respawnPoint.reset();
		player.addCreatureToTeamOrStorage(std::make_unique<CreatureUnit>(p_registries.creatures().get("sprout")));
		player.addCreatureToTeamOrStorage(std::make_unique<CreatureUnit>(p_registries.creatures().get("ember-fox")));
	}
}
