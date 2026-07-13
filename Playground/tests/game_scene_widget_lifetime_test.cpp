#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "game_scene_widget.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

namespace
{
	const pg::Registries &loadedRegistries()
	{
		static pg::Registries registries;
		static const bool loaded = [] {
			registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return true;
		}();
		(void)loaded;
		return registries;
	}
}

TEST(GameSceneWidgetLifetimeTest, ConstructionFailureAfterChunkWarmupLeavesContextSafe)
{
	pg::GameContext context;
	bool checkpointReached = false;
	pg::GameSceneConstructionOptions options;
	options.worldSeed = 1;
	options.writeWorldMapPreview = false;
	options.afterInitialWorldReady = [&](const pg::VoxelWorld &p_world, const pg::WorldNavigation &) {
		checkpointReached = true;
		EXPECT_GT(p_world.loadedChunkCount(), 0u);
		EXPECT_EQ(context.world.world, nullptr);
		EXPECT_EQ(context.world.navigation, nullptr);
		throw std::runtime_error("injected scene-construction failure");
	};

	EXPECT_THROW(
		{
			pg::GameSceneWidget widget("GameSceneFailure", nullptr, context, loadedRegistries(), options);
		},
		std::runtime_error);

	EXPECT_TRUE(checkpointReached);
	EXPECT_EQ(context.world.world, nullptr);
	EXPECT_EQ(context.world.navigation, nullptr);
	EXPECT_FALSE(context.world.explorationActive);

	// This is the original reproducer's dangerous final step. It must remain harmless
	// after the failed widget (and therefore its engine) has finished unwinding.
	context.world.navigation.reset();
	context.world.world.reset();
}

TEST(GameSceneWidgetLifetimeTest, SuccessfulDestructionClearsLoadedWorldBeforeEngineTeardown)
{
	pg::GameContext context;
	bool checkpointReached = false;
	pg::GameSceneConstructionOptions options;
	options.worldSeed = 1;
	options.writeWorldMapPreview = false;
	options.afterInitialWorldReady = [&](const pg::VoxelWorld &p_world, const pg::WorldNavigation &) {
		checkpointReached = true;
		EXPECT_GT(p_world.loadedChunkCount(), 0u);
	};

	{
		pg::GameSceneWidget widget("GameSceneSuccess", nullptr, context, loadedRegistries(), options);
		EXPECT_TRUE(checkpointReached);
		ASSERT_NE(context.world.world, nullptr);
		ASSERT_NE(context.world.navigation, nullptr);
		EXPECT_GT(context.world.world->loadedChunkCount(), 0u);
		EXPECT_TRUE(context.world.explorationActive);
	}

	EXPECT_EQ(context.world.navigation, nullptr);
	EXPECT_EQ(context.world.world, nullptr);
	EXPECT_FALSE(context.world.explorationActive);
}
