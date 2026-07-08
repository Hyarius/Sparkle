#include <gtest/gtest.h>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_prefab.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

#include <memory>
#include <stdexcept>

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
}

TEST(Prefab, RejectsVoxelsOutsideItsBox)
{
	spk::Prefab prefab({2, 2, 2});

	EXPECT_NO_THROW(prefab.addVoxel({0, 0, 0}, {0}));
	EXPECT_NO_THROW(prefab.addVoxel({1, 1, 1}, {0}));
	EXPECT_THROW(prefab.addVoxel({2, 0, 0}, {0}), std::out_of_range);
	EXPECT_THROW(prefab.addVoxel({0, -1, 0}, {0}), std::out_of_range);

	spk::Prefab empty;
	EXPECT_THROW(empty.addVoxel({0, 0, 0}, {0}), std::out_of_range);
}

TEST(Prefab, AddVoxelRangeFillsAnInclusiveBox)
{
	spk::Prefab prefab({4, 3, 5});
	const spk::VoxelCell cell{.id = 7, .orientation = spk::VoxelOrientation::NegativeZ};

	prefab.addVoxelRange({2, 1, 3}, {1, 0, 1}, cell);

	ASSERT_EQ(prefab.voxels().size(), 12u);
	std::size_t index = 0;
	for (int y = 0; y <= 1; ++y)
	{
		for (int z = 1; z <= 3; ++z)
		{
			for (int x = 1; x <= 2; ++x)
			{
				EXPECT_EQ(prefab.voxels()[index++], (spk::Prefab::Voxel{.position = {x, y, z}, .cell = cell}));
			}
		}
	}
}

TEST(Prefab, AddVoxelRangeWithMatchingCornersAddsOneVoxel)
{
	spk::Prefab prefab({2, 2, 2});
	const spk::VoxelCell cell{.id = 3};

	prefab.addVoxelRange({1, 1, 1}, {1, 1, 1}, cell);

	ASSERT_EQ(prefab.voxels().size(), 1u);
	EXPECT_EQ(prefab.voxels().front(), (spk::Prefab::Voxel{.position = {1, 1, 1}, .cell = cell}));
}

TEST(Prefab, AddVoxelRangeRejectsTheWholeBoxBeforeAddingVoxels)
{
	spk::Prefab prefab({2, 2, 2});

	EXPECT_THROW(prefab.addVoxelRange({0, 0, 0}, {2, 1, 1}, spk::VoxelCell{.id = 3}), std::out_of_range);
	EXPECT_TRUE(prefab.voxels().empty());
}

TEST(Prefab, ListsEveryCellOfAGridIncludingEmpties)
{
	spk::VoxelGrid grid(spk::Vector3Int{2, 2, 2});
	grid.cell(1, 0, 1) = {7};

	const spk::Prefab prefab(grid);
	EXPECT_EQ(prefab.size(), grid.size());
	ASSERT_EQ(prefab.voxels().size(), 8u);

	int nonEmpty = 0;
	for (const spk::Prefab::Voxel &voxel : prefab.voxels())
	{
		if (!voxel.cell.isEmpty())
		{
			++nonEmpty;
			EXPECT_EQ(voxel.position, spk::Vector3Int(1, 0, 1));
			EXPECT_EQ(voxel.cell.id, 7);
		}
	}
	EXPECT_EQ(nonEmpty, 1);
}

TEST(Prefab, RotatedSizeSwapsHorizontalAxesOnQuarterTurns)
{
	spk::Prefab prefab({3, 2, 1});

	EXPECT_EQ(prefab.rotatedSize(spk::VoxelOrientation::PositiveZ), spk::Vector3Int(3, 2, 1));
	EXPECT_EQ(prefab.rotatedSize(spk::VoxelOrientation::NegativeZ), spk::Vector3Int(3, 2, 1));
	EXPECT_EQ(prefab.rotatedSize(spk::VoxelOrientation::PositiveX), spk::Vector3Int(1, 2, 3));
	EXPECT_EQ(prefab.rotatedSize(spk::VoxelOrientation::NegativeX), spk::Vector3Int(1, 2, 3));
}

TEST(Prefab, AppliesWithQuarterTurnRotations)
{
	// Two markers in a 2x1x3 box; the second carries its own orientation so the test
	// also proves cell orientations rotate along with positions.
	spk::Prefab prefab({2, 1, 3});
	prefab.addVoxel({0, 0, 0}, {1});
	prefab.addVoxel({1, 0, 2}, {2, spk::VoxelOrientation::PositiveX});

	struct Expectation
	{
		spk::VoxelOrientation applied;
		spk::Vector3Int first;
		spk::Vector3Int second;
		spk::VoxelOrientation rotatedCell;
	};
	const Expectation expectations[] = {
		{spk::VoxelOrientation::PositiveZ, {0, 0, 0}, {1, 0, 2}, spk::VoxelOrientation::PositiveX},
		{spk::VoxelOrientation::PositiveX, {0, 0, 1}, {2, 0, 0}, spk::VoxelOrientation::NegativeZ},
		{spk::VoxelOrientation::NegativeZ, {1, 0, 2}, {0, 0, 0}, spk::VoxelOrientation::NegativeX},
		{spk::VoxelOrientation::NegativeX, {2, 0, 0}, {0, 0, 1}, spk::VoxelOrientation::PositiveZ},
	};

	for (const Expectation &expectation : expectations)
	{
		spk::VoxelGrid grid(spk::Vector3Int{3, 1, 3});
		prefab.applyTo(grid, {0, 0, 0}, expectation.applied);

		EXPECT_EQ(grid.cell(expectation.first).id, 1);
		EXPECT_EQ(grid.cell(expectation.second).id, 2);
		EXPECT_EQ(grid.cell(expectation.second).orientation, expectation.rotatedCell);
	}
}

TEST(Prefab, CarvesListedEmptyCellsAndLeavesUnlistedOnesUntouched)
{
	spk::VoxelGrid grid(spk::Vector3Int{3, 3, 3}, {5});

	// The box covers two cells but only one is listed (as an explicit empty).
	spk::Prefab prefab({2, 1, 1});
	prefab.addVoxel({0, 0, 0}, {});

	prefab.applyTo(grid, {1, 1, 1});
	EXPECT_TRUE(grid.cell(1, 1, 1).isEmpty());
	EXPECT_EQ(grid.cell(2, 1, 1).id, 5);
}

TEST(Prefab, SkipsVoxelsLandingOutsideTheGrid)
{
	spk::VoxelGrid grid(spk::Vector3Int{2, 2, 2});

	spk::Prefab prefab({2, 1, 1});
	prefab.addVoxel({0, 0, 0}, {1});
	prefab.addVoxel({1, 0, 0}, {2});

	EXPECT_NO_THROW(prefab.applyTo(grid, {-1, 0, 0}));
	EXPECT_EQ(grid.cell(0, 0, 0).id, 2);
	EXPECT_TRUE(grid.cell(1, 0, 0).isEmpty());
}

TEST(VoxelMap, AppliesAPrefabAcrossChunksGeneratingThemOnDemand)
{
	const TestRegistry test;
	int generationCount = 0;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &) { ++generationCount; });

	spk::Prefab prefab({4, 1, 1});
	prefab.addVoxelRange({0, 0, 0}, {3, 0, 0}, {test.cube});

	map.applyPrefab(prefab, {14, 0, 0});
	EXPECT_EQ(generationCount, 2);
	EXPECT_EQ(map.loadedChunkCount(), 2u);
	for (int x = 14; x < 18; ++x)
	{
		EXPECT_EQ(map.cell({x, 0, 0}).id, test.cube);
	}
	EXPECT_TRUE(map.cell({13, 0, 0}).isEmpty());
	EXPECT_TRUE(map.cell({18, 0, 0}).isEmpty());
}

TEST(VoxelMap, AppliesAPrefabRotated)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {});

	spk::Prefab prefab({2, 1, 1});
	prefab.addVoxel({0, 0, 0}, {test.cube});
	prefab.addVoxel({1, 0, 0}, {test.cube});

	// A quarter turn: the 2x1x1 row lays along +Z and the cells' own orientation follows.
	map.applyPrefab(prefab, {4, 0, 4}, spk::VoxelOrientation::PositiveX);
	EXPECT_EQ(map.cell({4, 0, 4}).id, test.cube);
	EXPECT_EQ(map.cell({4, 0, 5}).id, test.cube);
	EXPECT_EQ(map.cell({4, 0, 4}).orientation, spk::VoxelOrientation::PositiveX);
	EXPECT_TRUE(map.cell({5, 0, 4}).isEmpty());
}

TEST(VoxelMap, PrefabEmptyCellsCarveGeneratedContent)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &p_chunk) {
		p_chunk.setCell({0, 0, 0}, {test.cube});
		p_chunk.setCell({1, 0, 0}, {test.cube});
	});

	spk::Prefab prefab({2, 1, 1});
	prefab.addVoxel({0, 0, 0}, {});

	map.applyPrefab(prefab, {0, 0, 0});
	EXPECT_TRUE(map.cell({0, 0, 0}).isEmpty());
	EXPECT_EQ(map.cell({1, 0, 0}).id, test.cube);
}

TEST(VoxelMap, ApplyingAnEmptyPrefabLoadsNothing)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {});

	const spk::Prefab prefab({2, 2, 2});
	map.applyPrefab(prefab, {0, 0, 0});
	EXPECT_EQ(map.loadedChunkCount(), 0u);
}

TEST(VoxelMap, ApplyingAPrefabFlagsNeighborChunksForRebake)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {});

	spk::VoxelChunk &neighbor = map.chunk({1, 0, 0});
	neighbor.renderer().synchronize();
	ASSERT_FALSE(neighbor.renderer().needsSynchronization());

	spk::Prefab prefab({1, 1, 1});
	prefab.addVoxel({0, 0, 0}, {test.cube});
	map.applyPrefab(prefab, {15, 0, 0});

	EXPECT_TRUE(neighbor.renderer().needsSynchronization());
}
