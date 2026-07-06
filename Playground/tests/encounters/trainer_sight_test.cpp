#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "encounters/encounter_table.hpp"
#include "logics/trainer_sight_logic.hpp"
#include "world/map_definition.hpp"
#include "world/trainer.hpp"
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

	[[nodiscard]] pg::MapDefinition flatMap()
	{
		pg::MapDefinition map{.grid = pg::VoxelGrid({8, 4, 8})};
		const std::size_t ground = registries().voxels().numericId("grass-block");
		for (int z = 0; z < 8; ++z)
		{
			for (int x = 0; x < 8; ++x)
			{
				map.grid.cell(x, 0, z).id = ground;
			}
		}
		return map;
	}
}

TEST(TrainerSightLogic, StrictLineHonorsFacingRangeAndVoxelOcclusion)
{
	pg::VoxelWorld world(registries().voxels());
	world.loadFromMap(flatMap());
	const spk::Vector3Int trainer{2, 0, 1};
	EXPECT_TRUE(pg::TrainerSightLogic::canSee(world, trainer, pg::VoxelOrientation::PositiveZ, 5, {2, 0, 4}));
	EXPECT_FALSE(pg::TrainerSightLogic::canSee(world, trainer, pg::VoxelOrientation::PositiveZ, 5, {3, 0, 4}));
	EXPECT_FALSE(pg::TrainerSightLogic::canSee(world, trainer, pg::VoxelOrientation::PositiveZ, 5, {2, 0, 0}));
	EXPECT_FALSE(pg::TrainerSightLogic::canSee(world, trainer, pg::VoxelOrientation::PositiveZ, 5, {2, 0, 7}));
	EXPECT_TRUE(pg::TrainerSightLogic::canSee(world, {1, 0, 2}, pg::VoxelOrientation::PositiveX, 5, {4, 0, 2}));
	EXPECT_TRUE(pg::TrainerSightLogic::canSee(world, {4, 0, 2}, pg::VoxelOrientation::NegativeX, 5, {1, 0, 2}));
	EXPECT_TRUE(pg::TrainerSightLogic::canSee(world, {2, 0, 4}, pg::VoxelOrientation::NegativeZ, 5, {2, 0, 1}));

	ASSERT_TRUE(world.setCell({2, 1, 2}, {registries().voxels().numericId("wall-stone")}));
	EXPECT_FALSE(pg::TrainerSightLogic::canSee(world, trainer, pg::VoxelOrientation::PositiveZ, 5, {2, 0, 4}));
}

TEST(TrainerSightLogic, TriggerApproachesDisablesTamingAndVictoryClearsPermanently)
{
	pg::GameContext game;
	game.newGame(registries());
	game.world.explorationActive = true;
	pg::VoxelWorld world(registries().voxels());
	world.loadFromMap(flatMap());
	pg::WorldNavigation navigation(world, {{0, 0, 0}, {8, 4, 8}});
	pg::Actor actor;
	actor.cell = {2, 0, 1};
	pg::Trainer trainer;
	trainer.id = "test-trainer";
	trainer.actor = &actor;
	trainer.encounterTable = &registries().encounterTables().get("trainer-timmy");
	trainer.facing = pg::VoxelOrientation::PositiveZ;
	trainer.sightRange = 5;
	trainer.clearedFlag = "test-trainer-cleared";
	trainer.boardSize = {11, 9};
	pg::TrainerSightLogic sight(game, world, navigation, {11, 11});
	sight.addTrainer(trainer);
	std::vector<pg::EncounterSpawn> spawns;
	auto contract = game.events.encounterTriggered.subscribe(
		[&](const pg::EncounterSpawn &p_spawn) { spawns.push_back(p_spawn); });

	game.events.playerMoved.trigger({2, 0, 4});

	ASSERT_EQ(spawns.size(), 1);
	EXPECT_FALSE(spawns.front().allowsTaming);
	EXPECT_EQ(spawns.front().placementStyle, pg::PlacementStyle::ByLine);
	EXPECT_EQ(spawns.front().boardSize, spk::Vector2Int(11, 9));
	EXPECT_EQ(actor.cell, spk::Vector3Int(2, 0, 3));
	game.events.battleResolved.trigger(nullptr, pg::BattleSide::Player);
	EXPECT_TRUE(game.clearedTrainers.contains("test-trainer-cleared"));
	game.events.playerMoved.trigger({2, 0, 4});
	EXPECT_EQ(spawns.size(), 1);
	(void)contract;
}
