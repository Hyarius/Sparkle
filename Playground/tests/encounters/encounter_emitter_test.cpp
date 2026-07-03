#include "core/exploration_mode.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "encounters/biome.hpp"
#include "encounters/encounter_emitter.hpp"
#include "encounters/encounter_table.hpp"
#include "world/map_definition.hpp"
#include "world/voxel_world.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <vector>

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

	struct EmitterFixture
	{
		pg::GameContext context;
		pg::EncounterTable table;
		pg::BiomeDefinition biome;
		pg::MapDefinition map{.grid = pg::VoxelGrid({2, 4, 1})};
		std::vector<pg::EncounterSpawn> spawns;
		spk::ContractProvider<const pg::EncounterSpawn &>::Contract contract;

		EmitterFixture()
		{
			table.tiers[0].weightedTeams.push_back({.displayName = "lone sprout", .weight = 1, .team = {{.speciesId = "sprout", .aiId = "test", .completedNodeUuids = {}}}});
			biome.encounterRules.push_back({.trigger = "bush", .table = &table, .chancePerStep = 1.0});
			map.grid.cell(0, 0, 0).id = registries().voxels().numericId("grass-block");
			map.grid.cell(0, 1, 0).id = registries().voxels().numericId("bush");
			map.grid.cell(1, 0, 0).id = registries().voxels().numericId("grass-block");
			context.world.activeMap = &map;
			context.world.activeBiome = &biome;
			context.world.world = std::make_unique<pg::VoxelWorld>(registries().voxels());
			context.world.world->loadFromMap(map);
			context.world.encounterEmitter = std::make_unique<pg::EncounterEmitter>(context, spk::Vector2Int{11, 11});
			contract = context.events.encounterTriggered.subscribe([this](const pg::EncounterSpawn &p_spawn) {
				spawns.push_back(p_spawn);
			});
		}
	};
}

TEST(EncounterEmitter, FiresOnlyWhileExplorationModeIsActive)
{
	EmitterFixture fixture;
	fixture.context.events.playerMoved.trigger({0, 0, 0});
	EXPECT_TRUE(fixture.spawns.empty());

	pg::ExplorationMode mode(fixture.context);
	mode.enter();
	mode.exit();
	fixture.context.events.playerMoved.trigger({0, 0, 0});
	EXPECT_TRUE(fixture.spawns.empty());

	mode.enter();
	fixture.context.events.playerMoved.trigger({0, 0, 0});
	mode.exit();

	ASSERT_EQ(fixture.spawns.size(), 1);
	EXPECT_EQ(fixture.spawns.front().enemyTeam.front().speciesId, "sprout");
	EXPECT_EQ(pg::encounterSummary(fixture.spawns.front()), "Encounter: lone sprout (sprout) \xE2\x80\x94 board 11\xC3\x97"
															"11");
}

TEST(EncounterEmitter, ReadsTriggerTagFromPassableOccupancyCell)
{
	EmitterFixture fixture;
	pg::ExplorationMode mode(fixture.context);
	mode.enter();

	fixture.context.events.playerMoved.trigger({1, 0, 0});
	EXPECT_TRUE(fixture.spawns.empty());
	fixture.context.events.playerMoved.trigger({0, 0, 0});
	EXPECT_EQ(fixture.spawns.size(), 1);
	mode.exit();
}

TEST(EncounterEmitter, BiomeWithoutRulesNeverRolls)
{
	EmitterFixture fixture;
	fixture.biome.encounterRules.clear();
	pg::ExplorationMode mode(fixture.context);
	mode.enter();
	fixture.context.events.playerMoved.trigger({0, 0, 0});
	mode.exit();

	EXPECT_TRUE(fixture.spawns.empty());
}
