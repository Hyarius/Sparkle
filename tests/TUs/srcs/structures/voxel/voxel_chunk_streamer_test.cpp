#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_chunk_render_logic.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer_logic.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

namespace
{
	// Full engine wiring: streamer + render logics dispatched through the real
	// prioritized logic registry, exactly as a game would run them.
	struct StreamedWorld
	{
		spk::VoxelRegistry registry;
		std::int32_t cube = 0;
		int generationCount = 0;

		spk::GameEngine engine;
		spk::VoxelChunkStreamerLogic *streamerLogic = nullptr;
		spk::VoxelChunkRenderLogic *renderLogic = nullptr;

		spk::Camera3D camera;
		spk::Texture texture;
		spk::VoxelMap map;

		spk::Entity3D player;
		spk::VoxelChunkStreamer *streamer = nullptr;

		explicit StreamedWorld(bool p_retainInactiveChunks = true) :
			cube(registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}))),
			map(
				registry,
				[this](spk::VoxelChunk &p_chunk) {
					++generationCount;
					p_chunk.setCell({0, 0, 0}, {cube});
				},
				&engine)
		{
			streamerLogic = &engine.add<spk::VoxelChunkStreamerLogic>(p_retainInactiveChunks);
			renderLogic = &engine.add<spk::VoxelChunkRenderLogic>(texture, false);
			camera.makeMain();
			streamer = &player.addComponent<spk::VoxelChunkStreamer>(map);
			engine.addEntity(&player);
		}

		void tick()
		{
			engine.logicRegistry().update(spk::UpdateTick{}, engine.componentRegistry());
		}

		[[nodiscard]] spk::RenderUnitBuilder render()
		{
			spk::RenderUnitBuilder builder;
			engine.logicRegistry().render(builder, engine.componentRegistry());
			return builder;
		}

		[[nodiscard]] bool isChunkActive(const spk::Vector3Int &p_coordinates)
		{
			const spk::VoxelChunk *chunk = map.tryChunk(p_coordinates);
			return chunk != nullptr && chunk->isActivated();
		}
	};
}

TEST(VoxelChunkStreamer, StreamsBakesAndRendersTheWindowInOneTick)
{
	StreamedWorld world;
	world.streamer->setViewRange({1, 0, 1});

	// The streamer logic outranks the render logic, so a single tick is enough to
	// stream the chunks in, bake their meshes and emit their draw commands.
	EXPECT_GT(world.streamerLogic->priority(), world.renderLogic->priority());

	world.tick();
	const spk::RenderUnitBuilder builder = world.render();

	EXPECT_EQ(world.map.loadedChunkCount(), 9u);
	EXPECT_EQ(world.generationCount, 9);
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));
	EXPECT_TRUE(world.isChunkActive({-1, 0, 1}));
	EXPECT_FALSE(world.streamer->needsStreaming());
	EXPECT_FALSE(builder.empty());
	EXPECT_EQ(builder.size(), 9u);
}

TEST(VoxelChunkStreamer, MovingTheOriginSlidesTheActiveWindow)
{
	StreamedWorld world;
	world.streamer->setViewRange({1, 0, 1});
	world.tick();

	world.streamer->setOriginPosition({3, 0, 0});
	world.tick();

	// The two windows do not overlap: everything got generated once, the old window is
	// deactivated but stays loaded, the new one is active.
	EXPECT_EQ(world.map.loadedChunkCount(), 18u);
	EXPECT_EQ(world.generationCount, 18);
	EXPECT_FALSE(world.isChunkActive({0, 0, 0}));
	EXPECT_NE(world.map.tryChunk({0, 0, 0}), nullptr);
	EXPECT_TRUE(world.isChunkActive({3, 0, 0}));
	EXPECT_TRUE(world.isChunkActive({2, 0, -1}));

	// Coming back reactivates the retained chunks without regenerating anything.
	world.streamer->setOriginPosition({0, 0, 0});
	world.tick();
	EXPECT_EQ(world.generationCount, 18);
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));
	EXPECT_FALSE(world.isChunkActive({3, 0, 0}));
}

TEST(VoxelChunkStreamer, CanEvictChunksOutsideTheActiveWindow)
{
	StreamedWorld world(false);
	world.streamer->setViewRange({1, 0, 1});
	world.tick();
	ASSERT_EQ(world.map.loadedChunkCount(), 9u);

	world.streamer->setOriginPosition({3, 0, 0});
	world.tick();

	EXPECT_FALSE(world.streamerLogic->retainsInactiveChunks());
	EXPECT_EQ(world.map.loadedChunkCount(), 9u);
	EXPECT_EQ(world.map.tryChunk({0, 0, 0}), nullptr);
	EXPECT_NE(world.map.tryChunk({3, 0, 0}), nullptr);
}

TEST(VoxelChunkStreamer, ChangingRetentionPolicyTriggersReconciliation)
{
	StreamedWorld world;
	world.tick();
	(void)world.map.chunk({10, 0, 10});
	ASSERT_NE(world.map.tryChunk({10, 0, 10}), nullptr);

	world.streamerLogic->setRetainInactiveChunks(false);
	world.tick();

	EXPECT_EQ(world.map.tryChunk({10, 0, 10}), nullptr);
}

TEST(VoxelChunkStreamer, ReconcilesOnlyWhenAStreamerChanges)
{
	StreamedWorld world;
	world.tick();
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));

	// A chunk loaded manually outside the window stays active as long as no streamer
	// state changes...
	(void)world.map.chunk({10, 0, 10});
	world.tick();
	EXPECT_TRUE(world.isChunkActive({10, 0, 10}));

	// ...and is reconciled away as soon as one does.
	world.streamer->setOriginPosition({0, 0, 1});
	world.tick();
	EXPECT_FALSE(world.isChunkActive({10, 0, 10}));
	EXPECT_TRUE(world.isChunkActive({0, 0, 1}));
}

TEST(VoxelChunkStreamer, SeveralStreamersKeepTheUnionOfTheirWindowsActive)
{
	StreamedWorld world;
	spk::Entity3D secondPlayer;
	spk::VoxelChunkStreamer &secondStreamer = secondPlayer.addComponent<spk::VoxelChunkStreamer>(world.map);
	world.engine.addEntity(&secondPlayer);
	secondStreamer.setOriginPosition({5, 0, 5});

	world.tick();
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));
	EXPECT_TRUE(world.isChunkActive({5, 0, 5}));

	// Only the first streamer moves: the second one's window must survive the
	// reconciliation untouched.
	world.streamer->setOriginPosition({1, 0, 0});
	world.tick();
	EXPECT_FALSE(world.isChunkActive({0, 0, 0}));
	EXPECT_TRUE(world.isChunkActive({1, 0, 0}));
	EXPECT_TRUE(world.isChunkActive({5, 0, 5}));
}

TEST(VoxelChunkStreamer, RejectsNegativeViewRanges)
{
	StreamedWorld world;

	EXPECT_THROW(world.streamer->setViewRange({-1, 0, 0}), std::invalid_argument);
	EXPECT_THROW(world.streamer->setViewRange({0, -2, 0}), std::invalid_argument);
	EXPECT_EQ(world.streamer->viewRange(), spk::Vector3Int(0, 0, 0));
}
