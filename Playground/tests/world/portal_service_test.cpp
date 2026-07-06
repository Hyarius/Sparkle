#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
#include "encounters/encounter_emitter.hpp"
#include "world/portal_service.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries loaded;
			loaded.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return loaded;
		}();
		return result;
	}

	struct PortalFixture
	{
		pg::GameContext game;
		pg::Actor player;
		std::unique_ptr<pg::PortalService> service;

		PortalFixture()
		{
			game.newGame(registries());
			game.world.activeMap = &registries().maps().get("m1-testground");
			game.world.activeBiome = &registries().biomes().get(game.world.activeMap->biome);
			game.world.world = std::make_unique<pg::VoxelWorld>(registries().voxels());
			game.world.world->loadFromMap(*game.world.activeMap);
			game.world.navigation = std::make_unique<pg::WorldNavigation>(
				*game.world.world,
				pg::TraversalBounds{{0, 0, 0}, game.world.activeMap->size()});
			player.player = true;
			player.cell = {32, 2, 40};
			game.world.explorationActive = true;
			service = std::make_unique<pg::PortalService>(game, registries(), player);
		}
	};
}

TEST(PortalService, SwapsMapsLandsAtNamedTargetsAndCachesLoadedMaps)
{
	PortalFixture fixture;
	EXPECT_EQ(fixture.game.world.world->loadedMapCount(), 1u);

	fixture.game.events.playerMoved.trigger({12, 2, 30});
	EXPECT_EQ(fixture.game.world.activeMap->id, "test-tunnel");
	EXPECT_EQ(fixture.player.cell, spk::Vector3Int(0, 0, 2));
	EXPECT_EQ(fixture.game.world.world->loadedMapCount(), 2u);

	fixture.service->transitionToPortal("m1-testground", "tunnel-east");
	EXPECT_EQ(fixture.player.cell, spk::Vector3Int(52, 2, 30));
	fixture.game.events.playerMoved.trigger({52, 2, 30});
	EXPECT_EQ(fixture.game.world.activeMap->id, "test-tunnel");
	EXPECT_EQ(fixture.player.cell, spk::Vector3Int(12, 0, 2));
	EXPECT_EQ(fixture.game.world.world->loadedMapCount(), 2u);

	fixture.service->transitionToPortal("m1-testground", "house-door");
	fixture.game.events.playerMoved.trigger({43, 3, 18});
	EXPECT_EQ(fixture.game.world.activeMap->id, "small-house-interior");
	EXPECT_EQ(fixture.player.cell, spk::Vector3Int(4, 0, 0));
	EXPECT_EQ(fixture.game.world.world->loadedMapCount(), 3u);
}

TEST(PortalService, HealPointSetsRespawnAndLossReturnsHealedWithoutChangingFeats)
{
	PortalFixture fixture;
	pg::CreatureUnit &lead = *fixture.game.player.team[0];
	lead.currentHealth = 1;
	const nlohmann::json featProgress = lead.featBoardProgress.toJson();

	fixture.game.events.playerMoved.trigger({30, 4, 30});
	ASSERT_TRUE(fixture.game.respawnPoint.has_value());
	EXPECT_EQ(*fixture.game.respawnPoint, (pg::WorldPosition{.map = "m1-testground", .cell = {30, 4, 30}}));
	EXPECT_EQ(lead.currentHealth, lead.attributes.health);

	fixture.service->transitionToPortal("small-house-interior", "entrance");
	lead.currentHealth = 0;
	fixture.game.events.battleResolved.trigger(nullptr, pg::BattleSide::Enemy);
	EXPECT_EQ(fixture.game.world.activeMap->id, "m1-testground");
	EXPECT_EQ(fixture.player.cell, spk::Vector3Int(30, 4, 30));
	EXPECT_EQ(lead.currentHealth, lead.attributes.health);
	EXPECT_EQ(lead.featBoardProgress.toJson(), featProgress);
}

TEST(EncounterEmitter, CaveFloorRuleFiresInsideTunnel)
{
	PortalFixture fixture;
	fixture.service->transitionToPortal("test-tunnel", "west");
	fixture.game.world.encounterEmitter = std::make_unique<pg::EncounterEmitter>(
		fixture.game, spk::Vector2Int{11, 11});
	fixture.game.world.encounterEmitter->setEnabled(true);
	int encounters = 0;
	auto contract = fixture.game.events.encounterTriggered.subscribe(
		[&](const pg::EncounterSpawn &) { ++encounters; });

	fixture.game.events.playerMoved.trigger({5, 0, 2});
	EXPECT_EQ(encounters, 1);
}
