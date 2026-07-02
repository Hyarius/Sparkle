#include "core/registries.hpp"
#include "logics/chunk_synchronization_logic.hpp"
#include "world/chunk_provider.hpp"
#include "world/map_definition.hpp"
#include "world/voxel_world.hpp"
#include "world/world_streamer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>

namespace
{
	class EmptyProvider final : public pg::IChunkProvider
	{
	public:
		void fill(pg::Chunk &p_chunk) const override
		{
			p_chunk.requestSynchronization();
		}
	};

	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries loaded;
			loaded.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return loaded;
		}();
		return result;
	}

	[[nodiscard]] pg::VoxelCell stone()
	{
		return {registries().voxels().numericId("stone-block")};
	}
}

TEST(ChunkCoordinates, RoundTripsPositiveAndNegativeWorldCells)
{
	const std::array values = {-33, -17, -16, -15, -1, 0, 1, 15, 16, 17, 32};
	for (int x : values)
	{
		for (int z : values)
		{
			const spk::Vector3Int world{x, -1, z};
			const pg::ChunkCoordinates chunk = pg::ChunkCoordinates::fromWorldCell(world);
			const spk::Vector3Int local = chunk.localFromWorld(world);
			EXPECT_GE(local.x, 0);
			EXPECT_LT(local.x, pg::Chunk::Size.x);
			EXPECT_GE(local.y, 0);
			EXPECT_LT(local.y, pg::Chunk::Size.y);
			EXPECT_GE(local.z, 0);
			EXPECT_LT(local.z, pg::Chunk::Size.z);
			EXPECT_EQ(chunk.worldOrigin() + local, world);
		}
	}
}

TEST(VoxelWorld, ProvidesContinuousAccessAcrossChunkBorders)
{
	pg::VoxelWorld world(registries().voxels());
	const pg::MapDefinition &map = registries().maps().get("m1-testground");
	world.loadFromMap(map);

	for (const spk::Vector3Int position : {spk::Vector3Int{15, 2, 15}, spk::Vector3Int{16, 2, 15},
			 spk::Vector3Int{15, 2, 16}, spk::Vector3Int{16, 2, 16}})
	{
		EXPECT_EQ(world.cell(position), map.grid.cell(position));
	}
	EXPECT_TRUE(world.cell({-1, 0, 0}).isEmpty());
	EXPECT_TRUE(world.cell({64, 0, 0}).isEmpty());
}

TEST(VoxelWorld, BorderEditRequestsExactlyTouchedAndAdjacentChunks)
{
	pg::VoxelWorld world(registries().voxels());
	EmptyProvider provider;
	pg::Chunk &left = world.loadChunk({{0, 0, 0}}, provider);
	pg::Chunk &right = world.loadChunk({{1, 0, 0}}, provider);
	left.synchronize();
	right.synchronize();

	ASSERT_TRUE(world.setCell({15, 1, 1}, stone()));
	EXPECT_TRUE(left.needsSynchronization());
	EXPECT_TRUE(right.needsSynchronization());
	left.synchronize();
	right.synchronize();

	ASSERT_TRUE(world.setCell({2, 1, 1}, stone()));
	EXPECT_TRUE(left.needsSynchronization());
	EXPECT_FALSE(right.needsSynchronization());
}

TEST(VoxelWorld, ChunkBakerClearsDirtyFlagAndRebuildsMesh)
{
	pg::VoxelWorld world(registries().voxels());
	EmptyProvider provider;
	pg::Chunk &chunk = world.loadChunk({{0, 0, 0}}, provider);
	chunk.synchronize();
	ASSERT_TRUE(world.setCell({1, 1, 1}, stone()));

	pg::ChunkSynchronizationLogic baker;
	baker.process(chunk);
	EXPECT_FALSE(chunk.needsSynchronization());
	EXPECT_EQ(chunk.renderMesh().nbShape(), 6);
}

TEST(VoxelWorld, MeshingCullsFacesAcrossChunkBoundaries)
{
	pg::VoxelWorld world(registries().voxels());
	EmptyProvider provider;
	pg::Chunk &left = world.loadChunk({{0, 0, 0}}, provider);
	pg::Chunk &right = world.loadChunk({{1, 0, 0}}, provider);
	left.synchronize();
	right.synchronize();

	ASSERT_TRUE(world.setCell({15, 1, 1}, stone()));
	ASSERT_TRUE(world.setCell({16, 1, 1}, stone()));
	left.synchronize();
	right.synchronize();
	EXPECT_EQ(left.renderMesh().nbShape(), 5);
	EXPECT_EQ(right.renderMesh().nbShape(), 5);
}

TEST(MapChunkProvider, SliceMatchesDirectMapReads)
{
	const pg::MapDefinition &map = registries().maps().get("m1-testground");
	pg::MapChunkProvider provider(map);
	pg::VoxelWorld world(registries().voxels());
	pg::Chunk &chunk = world.loadChunk({{1, 0, 2}}, provider);
	const spk::Vector3Int origin = chunk.coordinates().worldOrigin();

	for (const spk::Vector3Int local : {spk::Vector3Int{0, 0, 0}, spk::Vector3Int{15, 2, 15},
			 spk::Vector3Int{8, 4, 0}, spk::Vector3Int{11, 4, 7}})
	{
		EXPECT_EQ(chunk.grid().cell(local), map.grid.cell(origin + local));
	}
}

TEST(WorldStreamer, ReplacesLoadedSetAroundNewFocus)
{
	pg::VoxelWorld world(registries().voxels());
	EmptyProvider provider;
	pg::WorldStreamer streamer(world, provider, {0, 0, 0});
	streamer.update({0, 0, 0});
	ASSERT_EQ(world.loadedChunkCount(), 1);
	EXPECT_NE(world.chunk({{0, 0, 0}}), nullptr);

	streamer.update({32, 0, -1});
	ASSERT_EQ(world.loadedChunkCount(), 1);
	EXPECT_EQ(world.chunk({{0, 0, 0}}), nullptr);
	EXPECT_NE(world.chunk({{2, 0, -1}}), nullptr);
}
