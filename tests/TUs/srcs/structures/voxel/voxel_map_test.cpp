#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>

#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

namespace
{
	struct TestRegistry
	{
		spk::VoxelRegistry registry;
		std::int32_t cube = 0;

		TestRegistry()
		{
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
		}
	};

	void synchronizeAll(spk::VoxelMap &p_map)
	{
		p_map.forEachLoadedChunk([](spk::VoxelChunk &p_chunk) {
			if (p_chunk.renderer().needsSynchronization())
			{
				p_chunk.renderer().synchronize();
			}
		});
	}
}

TEST(VoxelMap, RequiresAGenerator)
{
	const TestRegistry test;

	EXPECT_THROW(spk::VoxelMap(test.registry, nullptr), std::invalid_argument);
}

TEST(VoxelMap, GeneratesChunksLazilyAndOnlyOnce)
{
	const TestRegistry test;
	int generationCount = 0;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &p_chunk) {
		++generationCount;
		p_chunk.setCell({0, 0, 0}, {test.cube});
	});

	EXPECT_EQ(map.loadedChunkCount(), 0u);
	EXPECT_EQ(map.tryChunk({0, 0, 0}), nullptr);

	spk::VoxelChunk &loaded = map.chunk({0, 0, 0});
	EXPECT_EQ(generationCount, 1);
	EXPECT_EQ(map.loadedChunkCount(), 1u);
	EXPECT_EQ(loaded.cell({0, 0, 0}).id, test.cube);

	spk::VoxelChunk &again = map.chunk({0, 0, 0});
	EXPECT_EQ(&again, &loaded);
	EXPECT_EQ(generationCount, 1);
}

TEST(VoxelMap, RollsBackAChunkWhenGenerationThrows)
{
	const TestRegistry test;
	spk::GameEngine engine;
	int attempts = 0;
	spk::VoxelMap map(
		test.registry,
		[&](spk::VoxelChunk &p_chunk) {
			if (++attempts == 1)
			{
				throw std::runtime_error("generation failed");
			}
			p_chunk.setCell({0, 0, 0}, {test.cube});
		},
		&engine);

	EXPECT_THROW((void)map.chunk({0, 0, 0}), std::runtime_error);
	EXPECT_EQ(map.loadedChunkCount(), 0u);
	const auto &renderers = engine.componentRegistry().components<spk::VoxelChunkRenderer>();
	EXPECT_EQ(std::count_if(renderers.begin(), renderers.end(), [](const auto *p_renderer) {
				  return p_renderer != nullptr;
			  }),
			  0);

	spk::VoxelChunk &chunk = map.chunk({0, 0, 0});
	EXPECT_EQ(attempts, 2);
	EXPECT_EQ(chunk.cell({0, 0, 0}).id, test.cube);
}

TEST(VoxelMap, ChunksAreEntitiesPlacedAtTheirWorldOrigin)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {
	});

	spk::VoxelChunk &loaded = map.chunk({1, 2, -1});
	spk::Entity &entity = loaded;
	(void)entity;

	EXPECT_EQ(loaded.coordinates(), spk::Vector3Int(1, 2, -1));
	EXPECT_EQ(loaded.worldOrigin(), spk::Vector3Int(16, 32, -16));
	EXPECT_EQ(loaded.transform().position(), spk::Vector3(16.0f, 32.0f, -16.0f));
}

TEST(VoxelMap, MapsWorldCellsToChunkCoordinates)
{
	EXPECT_EQ(spk::VoxelChunk::coordinatesFromWorldCell({0, 0, 0}), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(spk::VoxelChunk::coordinatesFromWorldCell({15, 15, 15}), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(spk::VoxelChunk::coordinatesFromWorldCell({16, 0, -1}), spk::Vector3Int(1, 0, -1));
	EXPECT_EQ(spk::VoxelChunk::coordinatesFromWorldCell({-16, -17, 31}), spk::Vector3Int(-1, -2, 1));
}

TEST(VoxelMap, ReadsAndWritesCellsInLoadedChunksOnly)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &p_chunk) {
		p_chunk.setCell({0, 0, 0}, {test.cube});
	});

	(void)map.chunk({0, 0, 0});

	ASSERT_NE(map.tryCell({0, 0, 0}), nullptr);
	EXPECT_EQ(map.tryCell({0, 0, 0})->id, test.cube);
	EXPECT_TRUE(map.cell({1, 0, 0}).isEmpty());

	// The neighboring chunk is not loaded: reads fall back to empty, writes are refused.
	EXPECT_EQ(map.tryCell({16, 0, 0}), nullptr);
	EXPECT_TRUE(map.cell({16, 0, 0}).isEmpty());
	EXPECT_FALSE(map.setCell({16, 0, 0}, {test.cube}));

	EXPECT_TRUE(map.setCell({5, 5, 5}, {test.cube}));
	EXPECT_EQ(map.cell({5, 5, 5}).id, test.cube);
}

TEST(VoxelMap, BoundaryEditsDirtyTheNeighborChunk)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {
	});

	spk::VoxelChunk &origin = map.chunk({0, 0, 0});
	spk::VoxelChunk &neighbor = map.chunk({1, 0, 0});
	synchronizeAll(map);
	ASSERT_FALSE(origin.renderer().needsSynchronization());
	ASSERT_FALSE(neighbor.renderer().needsSynchronization());

	// An edit in the middle of the chunk only re-bakes its own mesh.
	EXPECT_TRUE(map.setCell({5, 5, 5}, {test.cube}));
	EXPECT_TRUE(origin.renderer().needsSynchronization());
	EXPECT_FALSE(neighbor.renderer().needsSynchronization());
	synchronizeAll(map);

	// An edit on the shared boundary re-bakes both chunks.
	EXPECT_TRUE(map.setCell({15, 5, 5}, {test.cube}));
	EXPECT_TRUE(origin.renderer().needsSynchronization());
	EXPECT_TRUE(neighbor.renderer().needsSynchronization());
}

TEST(VoxelMap, DirectChunkEditsInvalidateEachTouchedLoadedNeighbor)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {
	});
	spk::VoxelChunk &origin = map.chunk({0, 0, 0});
	const std::array offsets = {
		spk::Vector3Int{-1, 0, 0},
		spk::Vector3Int{1, 0, 0},
		spk::Vector3Int{0, -1, 0},
		spk::Vector3Int{0, 1, 0},
		spk::Vector3Int{0, 0, -1},
		spk::Vector3Int{0, 0, 1}};
	const std::array positions = {
		spk::Vector3Int{0, 1, 1},
		spk::Vector3Int{15, 2, 2},
		spk::Vector3Int{3, 0, 3},
		spk::Vector3Int{4, 15, 4},
		spk::Vector3Int{5, 5, 0},
		spk::Vector3Int{6, 6, 15}};
	for (const spk::Vector3Int &offset : offsets)
	{
		(void)map.chunk(offset);
	}
	synchronizeAll(map);

	origin.setCell({7, 7, 7}, {test.cube});
	EXPECT_TRUE(origin.renderer().needsSynchronization());
	for (const spk::Vector3Int &offset : offsets)
	{
		const spk::VoxelChunk *neighbor = map.tryChunk(offset);
		ASSERT_NE(neighbor, nullptr);
		EXPECT_FALSE(neighbor->renderer().needsSynchronization());
	}
	synchronizeAll(map);

	for (std::size_t index = 0; index < positions.size(); ++index)
	{
		origin.setCell(positions[index], {test.cube});
		EXPECT_TRUE(origin.renderer().needsSynchronization());
		for (std::size_t neighborIndex = 0; neighborIndex < offsets.size(); ++neighborIndex)
		{
			const spk::VoxelChunk *neighbor = map.tryChunk(offsets[neighborIndex]);
			ASSERT_NE(neighbor, nullptr);
			EXPECT_EQ(neighbor->renderer().needsSynchronization(), neighborIndex == index);
		}
		synchronizeAll(map);
	}
}

TEST(VoxelMap, ChunkEditAfterABakeCannotBypassInvalidation)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {
	});
	spk::VoxelChunk &chunk = map.chunk({0, 0, 0});
	static_assert(std::is_same_v<decltype(chunk.grid()), const spk::VoxelGrid &>);
	static_assert(std::is_same_v<decltype(std::declval<spk::VoxelChunk::Editor &>().cell(0, 0, 0)), const spk::VoxelCell &>);

	chunk.renderer().synchronize();
	ASSERT_FALSE(chunk.renderer().needsSynchronization());

	chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
		EXPECT_TRUE(p_editor.setCell(1, 2, 3, {test.cube}));
	});
	EXPECT_TRUE(chunk.renderer().needsSynchronization());
	chunk.renderer().synchronize();
	EXPECT_EQ(chunk.renderer().meshRevision(), 2u);

	chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
		EXPECT_FALSE(p_editor.setCell(1, 2, 3, {test.cube}));
	});
	EXPECT_FALSE(chunk.renderer().needsSynchronization());
}

TEST(VoxelMap, ChunkMutationIsRejectedOffItsOwningThread)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {
	});
	spk::VoxelChunk &chunk = map.chunk({0, 0, 0});
	std::atomic_bool rejected = false;
	std::thread worker([&]() {
		try
		{
			chunk.setCell({1, 1, 1}, {test.cube});
		} catch (const std::logic_error &)
		{
			rejected.store(true);
		}
	});
	worker.join();

	EXPECT_TRUE(rejected.load());
	EXPECT_TRUE(chunk.cell({1, 1, 1}).isEmpty());
}

TEST(VoxelMap, CullsFacesAcrossChunkBoundaries)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &p_chunk) {
		p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
			for (int y = 0; y < spk::VoxelChunk::Size.y; ++y)
			{
				for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
				{
					for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
					{
						(void)p_editor.setCell(x, y, z, {test.cube});
					}
				}
			}
		});
	});

	// A lone, fully solid 16x16x16 chunk exposes its hull: 6 sides of 256 faces.
	spk::VoxelChunk &origin = map.chunk({0, 0, 0});
	EXPECT_TRUE(origin.renderer().needsSynchronization());
	origin.renderer().synchronize();
	EXPECT_EQ(origin.renderer().meshes().opaque.nbShape(), 1536u);
	const std::uint64_t firstRevision = origin.renderer().meshRevision();

	// Loading the +X neighbor dirties the origin chunk; re-baking culls the shared wall
	// on both sides.
	spk::VoxelChunk &neighbor = map.chunk({1, 0, 0});
	EXPECT_TRUE(origin.renderer().needsSynchronization());
	synchronizeAll(map);
	EXPECT_EQ(origin.renderer().meshes().opaque.nbShape(), 1536u - 256u);
	EXPECT_EQ(neighbor.renderer().meshes().opaque.nbShape(), 1536u - 256u);
	EXPECT_GT(origin.renderer().meshRevision(), firstRevision);

	// Unloading the neighbor restores the origin's full hull.
	EXPECT_TRUE(map.unloadChunk({1, 0, 0}));
	EXPECT_FALSE(map.unloadChunk({1, 0, 0}));
	EXPECT_EQ(map.loadedChunkCount(), 1u);
	EXPECT_TRUE(origin.renderer().needsSynchronization());
	origin.renderer().synchronize();
	EXPECT_EQ(origin.renderer().meshes().opaque.nbShape(), 1536u);
}

TEST(VoxelMap, InactiveChunksDoNotOccludeActiveNeighbors)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &p_chunk) {
		p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
			for (int y = 0; y < spk::VoxelChunk::Size.y; ++y)
			{
				for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
				{
					for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
					{
						(void)p_editor.setCell(x, y, z, {test.cube});
					}
				}
			}
		});
	});

	spk::VoxelChunk &origin = map.chunk({0, 0, 0});
	(void)map.chunk({1, 0, 0});
	synchronizeAll(map);
	ASSERT_EQ(origin.renderer().meshes().opaque.nbShape(), 1536u - 256u);

	EXPECT_TRUE(map.setChunkActive({1, 0, 0}, false));
	ASSERT_TRUE(origin.renderer().needsSynchronization());
	origin.renderer().synchronize();
	EXPECT_EQ(origin.renderer().meshes().opaque.nbShape(), 1536u);

	EXPECT_TRUE(map.setChunkActive({1, 0, 0}, true));
	synchronizeAll(map);
	EXPECT_EQ(origin.renderer().meshes().opaque.nbShape(), 1536u - 256u);
}

TEST(VoxelMap, RegistersChunkEntitiesInTheGameEngine)
{
	const TestRegistry test;
	spk::GameEngine engine;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {
	},
					  &engine);

	const auto countRenderers = [&]() {
		const auto &components = engine.componentRegistry().components<spk::VoxelChunkRenderer>();
		return std::count_if(components.begin(), components.end(), [](const spk::Component *p_component) {
			return p_component != nullptr;
		});
	};

	(void)map.chunk({0, 0, 0});
	(void)map.chunk({0, 0, 1});
	EXPECT_EQ(countRenderers(), 2);

	EXPECT_TRUE(map.unloadChunk({0, 0, 1}));
	EXPECT_EQ(countRenderers(), 1);

	map.clear();
	EXPECT_EQ(countRenderers(), 0);
	EXPECT_EQ(map.loadedChunkCount(), 0u);
}
