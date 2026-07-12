#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <memory>
#include <new>
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
		spk::VoxelRuntimeId cube{};
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
			engine.logicRegistry().update(spk::UpdateContext{}, engine.componentRegistry());
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

TEST(VoxelChunkStreamer, RetentionPolicyDoesNotClaimManuallyLoadedChunks)
{
	StreamedWorld world;
	world.tick();
	(void)world.map.chunk({10, 0, 10});
	ASSERT_NE(world.map.tryChunk({10, 0, 10}), nullptr);

	world.streamerLogic->setRetainInactiveChunks(false);
	world.tick();

	EXPECT_NE(world.map.tryChunk({10, 0, 10}), nullptr);
	EXPECT_TRUE(world.isChunkActive({10, 0, 10}));
}

TEST(VoxelChunkStreamer, RepairsOwnedChunksButPreservesManualChunkState)
{
	StreamedWorld world;
	world.tick();
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));

	// A manually loaded chunk is never claimed merely because its map is streamed.
	(void)world.map.chunk({10, 0, 10});
	world.tick();
	EXPECT_TRUE(world.isChunkActive({10, 0, 10}));

	// The chunk loaded by the streamer remains owned, so a manual deactivation is
	// repaired even when the desired window itself is unchanged.
	world.map.setChunkActive({0, 0, 0}, false);
	ASSERT_FALSE(world.isChunkActive({0, 0, 0}));
	world.tick();
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));
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

TEST(VoxelChunkStreamer, RemovingLastStreamerDeactivatesRetainedChunks)
{
	StreamedWorld world(true);
	world.streamer->setViewRange({1, 0, 0});
	world.tick();
	ASSERT_EQ(world.map.loadedChunkCount(), 3u);

	world.player.removeComponent<spk::VoxelChunkStreamer>();
	world.streamer = nullptr;
	world.tick();

	EXPECT_EQ(world.map.loadedChunkCount(), 3u);
	EXPECT_FALSE(world.isChunkActive({-1, 0, 0}));
	EXPECT_FALSE(world.isChunkActive({0, 0, 0}));
	EXPECT_FALSE(world.isChunkActive({1, 0, 0}));
}

TEST(VoxelChunkStreamer, DeactivatingLastStreamerUnloadsChunksWhenRetentionIsDisabled)
{
	StreamedWorld world(false);
	world.streamer->setViewRange({1, 0, 0});
	world.tick();
	ASSERT_EQ(world.map.loadedChunkCount(), 3u);

	world.streamer->deactivate();
	world.tick();

	EXPECT_EQ(world.map.loadedChunkCount(), 0u);
	EXPECT_EQ(world.map.tryChunk({0, 0, 0}), nullptr);
}

TEST(VoxelChunkStreamer, DeactivatingAndReactivatingEntityRestoresItsWindow)
{
	StreamedWorld world(true);
	world.tick();
	ASSERT_TRUE(world.isChunkActive({0, 0, 0}));

	world.player.deactivate();
	world.tick();
	EXPECT_FALSE(world.isChunkActive({0, 0, 0}));

	world.player.activate();
	world.tick();
	EXPECT_TRUE(world.isChunkActive({0, 0, 0}));
	EXPECT_EQ(world.generationCount, 1);
}

TEST(VoxelChunkStreamer, RemovingStreamersOneAtATimePreservesTheRemainingUnion)
{
	StreamedWorld world;
	spk::Entity3D secondPlayer;
	spk::VoxelChunkStreamer &secondStreamer = secondPlayer.addComponent<spk::VoxelChunkStreamer>(world.map);
	world.engine.addEntity(&secondPlayer);
	secondStreamer.setOriginPosition({5, 0, 0});
	world.tick();
	ASSERT_TRUE(world.isChunkActive({0, 0, 0}));
	ASSERT_TRUE(world.isChunkActive({5, 0, 0}));

	world.player.removeComponent<spk::VoxelChunkStreamer>();
	world.streamer = nullptr;
	world.tick();
	EXPECT_FALSE(world.isChunkActive({0, 0, 0}));
	EXPECT_TRUE(world.isChunkActive({5, 0, 0}));

	secondPlayer.removeComponent<spk::VoxelChunkStreamer>();
	world.tick();
	EXPECT_FALSE(world.isChunkActive({5, 0, 0}));
}

TEST(VoxelChunkStreamer, ReusedMapAddressDoesNotReuseHistoricalStreamingState)
{
	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	spk::GameEngine engine;
	(void)engine.add<spk::VoxelChunkStreamerLogic>();

	alignas(spk::VoxelMap) std::byte mapStorage[sizeof(spk::VoxelMap)];
	auto generator = [&cube](spk::VoxelChunk &p_chunk) { p_chunk.setCell({0, 0, 0}, {cube}); };
	spk::VoxelMap *firstMap = std::construct_at(
		reinterpret_cast<spk::VoxelMap *>(mapStorage), registry, generator, &engine);
	spk::Entity3D firstPlayer;
	spk::VoxelChunkStreamer &expiredStreamer =
		firstPlayer.addComponent<spk::VoxelChunkStreamer>(*firstMap);
	engine.addEntity(&firstPlayer);
	expiredStreamer.setOriginPosition({10, 0, 0});
	engine.logicRegistry().update(spk::UpdateContext{}, engine.componentRegistry());
	ASSERT_NE(firstMap->tryChunk({10, 0, 0}), nullptr);

	std::destroy_at(firstMap);
	EXPECT_FALSE(expiredStreamer.hasLiveMap());
	EXPECT_THROW((void)expiredStreamer.map(), std::logic_error);

	spk::VoxelMap *secondMap = std::construct_at(
		reinterpret_cast<spk::VoxelMap *>(mapStorage), registry, generator, &engine);
	spk::Entity3D secondPlayer;
	spk::VoxelChunkStreamer &secondStreamer =
		secondPlayer.addComponent<spk::VoxelChunkStreamer>(*secondMap);
	engine.addEntity(&secondPlayer);
	secondStreamer.setOriginPosition({20, 0, 0});

	EXPECT_NO_THROW(engine.logicRegistry().update(spk::UpdateContext{}, engine.componentRegistry()));
	EXPECT_EQ(secondMap->tryChunk({10, 0, 0}), nullptr);
	EXPECT_NE(secondMap->tryChunk({20, 0, 0}), nullptr);

	engine.removeEntity(&secondPlayer);
	engine.removeEntity(&firstPlayer);
	std::destroy_at(secondMap);
}

TEST(VoxelChunkStreamer, RejectsUnrepresentableCoordinateBoundsTransactionally)
{
	StreamedWorld world;
	const spk::Vector3Int originalOrigin = world.streamer->originPosition();
	const spk::Vector3Int originalRange = world.streamer->viewRange();

	EXPECT_THROW(
		world.streamer->setOriginPosition({std::numeric_limits<std::int32_t>::max(), 0, 0}),
		std::out_of_range);
	EXPECT_THROW(
		world.streamer->setOriginPosition({std::numeric_limits<std::int32_t>::min(), 0, 0}),
		std::out_of_range);
	EXPECT_EQ(world.streamer->originPosition(), originalOrigin);

	const std::int32_t maximumSafe =
		std::numeric_limits<std::int32_t>::max() / spk::VoxelChunk::Size.x;
	world.streamer->setOriginPosition({maximumSafe, 0, 0});
	EXPECT_THROW(world.streamer->setViewRange({1, 0, 0}), std::out_of_range);
	EXPECT_EQ(world.streamer->viewRange(), originalRange);
}

TEST(VoxelChunkStreamer, DefinesIntegerBoundaryBehaviorOnEveryAxis)
{
	StreamedWorld world;
	const std::int32_t safeMinimum =
		std::numeric_limits<std::int32_t>::min() / spk::VoxelChunk::Size.x;
	const std::int32_t safeMaximum =
		std::numeric_limits<std::int32_t>::max() / spk::VoxelChunk::Size.x;
	const std::array axes = {
		spk::Vector3Int{1, 0, 0},
		spk::Vector3Int{0, 1, 0},
		spk::Vector3Int{0, 0, 1}};
	const auto along = [](const spk::Vector3Int &p_axis, std::int32_t p_value) {
		return spk::Vector3Int{p_axis.x * p_value, p_axis.y * p_value, p_axis.z * p_value};
	};

	for (const spk::Vector3Int &axis : axes)
	{
		spk::VoxelChunkStreamer rawBoundary(world.map);
		EXPECT_THROW(
			rawBoundary.setOriginPosition(along(axis, std::numeric_limits<std::int32_t>::min())),
			std::out_of_range);
		EXPECT_THROW(
			rawBoundary.setOriginPosition(along(axis, std::numeric_limits<std::int32_t>::min() + 1)),
			std::out_of_range);
		EXPECT_THROW(
			rawBoundary.setOriginPosition(along(axis, std::numeric_limits<std::int32_t>::max() - 1)),
			std::out_of_range);
		EXPECT_THROW(
			rawBoundary.setOriginPosition(along(axis, std::numeric_limits<std::int32_t>::max())),
			std::out_of_range);

		spk::VoxelChunkStreamer minimum(world.map);
		EXPECT_NO_THROW(minimum.setOriginPosition(along(axis, safeMinimum)));
		EXPECT_THROW(minimum.setViewRange(axis), std::out_of_range);

		spk::VoxelChunkStreamer adjacentMinimum(world.map);
		adjacentMinimum.setOriginPosition(along(axis, safeMinimum + 1));
		EXPECT_NO_THROW(adjacentMinimum.setViewRange(axis));

		spk::VoxelChunkStreamer maximum(world.map);
		EXPECT_NO_THROW(maximum.setOriginPosition(along(axis, safeMaximum)));
		EXPECT_THROW(maximum.setViewRange(axis), std::out_of_range);

		spk::VoxelChunkStreamer adjacentMaximum(world.map);
		adjacentMaximum.setOriginPosition(along(axis, safeMaximum - 1));
		EXPECT_NO_THROW(adjacentMaximum.setViewRange(axis));
	}
}

TEST(VoxelChunkStreamer, AcceptsLargestWindowAndRejectsFirstExcessiveOne)
{
	StreamedWorld world;
	constexpr std::int32_t largestAcceptedLinearRange =
		static_cast<std::int32_t>((spk::VoxelChunkStreamer::MaximumChunkWindowVolume - 1u) / 2u);

	EXPECT_NO_THROW(world.streamer->setViewRange({largestAcceptedLinearRange, 0, 0}));
	EXPECT_EQ(
		world.streamer->windowChunkCount(),
		spk::VoxelChunkStreamer::MaximumChunkWindowVolume - 1u);
	EXPECT_THROW(
		world.streamer->setViewRange({largestAcceptedLinearRange + 1, 0, 0}),
		std::length_error);
	EXPECT_EQ(world.streamer->viewRange(), spk::Vector3Int(largestAcceptedLinearRange, 0, 0));

	spk::VoxelChunkStreamer yAxis(world.map);
	EXPECT_NO_THROW(yAxis.setViewRange({0, largestAcceptedLinearRange, 0}));
	EXPECT_THROW(yAxis.setViewRange({0, largestAcceptedLinearRange + 1, 0}), std::length_error);
	spk::VoxelChunkStreamer zAxis(world.map);
	EXPECT_NO_THROW(zAxis.setViewRange({0, 0, largestAcceptedLinearRange}));
	EXPECT_THROW(zAxis.setViewRange({0, 0, largestAcceptedLinearRange + 1}), std::length_error);

	spk::VoxelChunkStreamer product(world.map);
	EXPECT_THROW(product.setViewRange({32, 32, 32}), std::length_error);
}

TEST(VoxelChunkStreamer, RejectsCombinedWindowsBeforeLoadingAnyChunks)
{
	StreamedWorld world;
	constexpr std::int32_t largeRange =
		static_cast<std::int32_t>((spk::VoxelChunkStreamer::MaximumChunkWindowVolume - 1u) / 2u);
	world.streamer->setViewRange({largeRange, 0, 0});

	spk::Entity3D secondPlayer;
	spk::VoxelChunkStreamer &second = secondPlayer.addComponent<spk::VoxelChunkStreamer>(world.map);
	second.setOriginPosition({3000, 0, 0});
	world.engine.addEntity(&secondPlayer);

	spk::Entity3D thirdPlayer;
	spk::VoxelChunkStreamer &third = thirdPlayer.addComponent<spk::VoxelChunkStreamer>(world.map);
	third.setOriginPosition({3001, 0, 0});
	world.engine.addEntity(&thirdPlayer);

	EXPECT_THROW(world.tick(), std::length_error);
	EXPECT_EQ(world.map.loadedChunkCount(), 0u);
	EXPECT_EQ(world.generationCount, 0);
}
