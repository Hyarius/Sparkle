#include <gtest/gtest.h>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_prefab.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

#include <memory>

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

TEST(Prefab, DeducesItsBoundsFromItsVoxels)
{
	spk::Prefab prefab;
	EXPECT_EQ(prefab.minBounds(), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(prefab.maxBounds(), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(prefab.size(), spk::Vector3Int(0, 0, 0));

	// Positions are trusted as authored, negative coordinates included.
	prefab.addVoxel({2, -1, 0}, {1});
	prefab.addVoxelRange({-1, 1, 1}, {0, 2, 3}, {1});
	EXPECT_EQ(prefab.minBounds(), spk::Vector3Int(-1, -1, 0));
	EXPECT_EQ(prefab.maxBounds(), spk::Vector3Int(2, 2, 3));
	EXPECT_EQ(prefab.size(), spk::Vector3Int(4, 4, 4));
}

TEST(Prefab, AddVoxelRangeFillsAnInclusiveBox)
{
	spk::Prefab prefab;
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
	spk::Prefab prefab;
	const spk::VoxelCell cell{.id = 3};

	prefab.addVoxelRange({1, 1, 1}, {1, 1, 1}, cell);

	ASSERT_EQ(prefab.voxels().size(), 1u);
	EXPECT_EQ(prefab.voxels().front(), (spk::Prefab::Voxel{.position = {1, 1, 1}, .cell = cell}));
}

TEST(Prefab, ListsEveryCellOfAGridIncludingEmpties)
{
	spk::VoxelGrid grid(spk::Vector3Int{2, 2, 2});
	grid.cell(1, 0, 1) = {7};

	const spk::Prefab prefab(grid);
	EXPECT_EQ(prefab.minBounds(), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(prefab.maxBounds(), spk::Vector3Int(1, 1, 1));
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

TEST(Prefab, GridConstructorPlacesTheBoxAtTheGivenOrigin)
{
	spk::VoxelGrid grid(spk::Vector3Int{1, 2, 1});
	grid.cell(0, 0, 0) = {7};

	const spk::Prefab prefab(grid, {0, -1, 0});
	EXPECT_EQ(prefab.minBounds(), spk::Vector3Int(0, -1, 0));
	EXPECT_EQ(prefab.maxBounds(), spk::Vector3Int(0, 0, 0));
	ASSERT_EQ(prefab.voxels().size(), 2u);
	EXPECT_EQ(prefab.voxels()[0], (spk::Prefab::Voxel{.position = {0, -1, 0}, .cell = {7}}));
	EXPECT_EQ(prefab.voxels()[1].position, spk::Vector3Int(0, 0, 0));
	EXPECT_TRUE(prefab.voxels()[1].cell.isEmpty());
}

TEST(Prefab, RotatedBoundsFollowTheOrientation)
{
	spk::Prefab prefab;
	prefab.addVoxelRange({0, -1, 0}, {2, 1, 0}, {1});

	const auto [identityMin, identityMax] = prefab.rotatedBounds(spk::VoxelOrientation::PositiveZ);
	EXPECT_EQ(identityMin, spk::Vector3Int(0, -1, 0));
	EXPECT_EQ(identityMax, spk::Vector3Int(2, 1, 0));

	// One quarter turn maps (x, z) to (z, -x): the row along +X swings to -Z.
	const auto [quarterMin, quarterMax] = prefab.rotatedBounds(spk::VoxelOrientation::PositiveX);
	EXPECT_EQ(quarterMin, spk::Vector3Int(0, -1, -2));
	EXPECT_EQ(quarterMax, spk::Vector3Int(0, 1, 0));
}

TEST(Prefab, AppliesWithQuarterTurnRotations)
{
	// Two markers; the second carries its own orientation so the test also proves cell
	// orientations rotate along with positions (around the default pivot, the origin).
	spk::Prefab prefab;
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
		{spk::VoxelOrientation::PositiveZ, {2, 0, 2}, {3, 0, 4}, spk::VoxelOrientation::PositiveX},
		{spk::VoxelOrientation::PositiveX, {2, 0, 2}, {4, 0, 1}, spk::VoxelOrientation::NegativeZ},
		{spk::VoxelOrientation::NegativeZ, {2, 0, 2}, {1, 0, 0}, spk::VoxelOrientation::NegativeX},
		{spk::VoxelOrientation::NegativeX, {2, 0, 2}, {0, 0, 3}, spk::VoxelOrientation::PositiveZ},
	};

	for (const Expectation &expectation : expectations)
	{
		spk::VoxelGrid grid(spk::Vector3Int{5, 1, 5});
		prefab.applyTo(grid, {2, 0, 2}, expectation.applied);

		EXPECT_EQ(grid.cell(expectation.first).id, 1);
		EXPECT_EQ(grid.cell(expectation.second).id, 2);
		EXPECT_EQ(grid.cell(expectation.second).orientation, expectation.rotatedCell);
	}
}

TEST(Prefab, RotatesAroundItsPivot)
{
	// The pivot cell lands on the destination for every orientation; its +X neighbor
	// swings around it.
	spk::Prefab prefab;
	prefab.addVoxel({1, 0, 1}, {1});
	prefab.addVoxel({2, 0, 1}, {2});
	prefab.setPivot({1, 0, 1});

	const std::pair<spk::VoxelOrientation, spk::Vector3Int> expectations[] = {
		{spk::VoxelOrientation::PositiveZ, {2, 0, 1}},
		{spk::VoxelOrientation::PositiveX, {1, 0, 0}},
		{spk::VoxelOrientation::NegativeZ, {0, 0, 1}},
		{spk::VoxelOrientation::NegativeX, {1, 0, 2}},
	};
	for (const auto &[applied, second] : expectations)
	{
		spk::VoxelGrid grid(spk::Vector3Int{3, 1, 3});
		prefab.applyTo(grid, {1, 0, 1}, applied);

		EXPECT_EQ(grid.cell(1, 0, 1).id, 1);
		EXPECT_EQ(grid.cell(second).id, 2);
	}
}

TEST(Prefab, NegativeYLayersLandBelowTheDestination)
{
	spk::VoxelGrid grid(spk::Vector3Int{3, 3, 3});
	spk::Prefab prefab;
	prefab.addVoxel({0, -1, 0}, {1});
	prefab.addVoxel({0, 0, 0}, {2});

	prefab.applyTo(grid, {1, 1, 1}, spk::VoxelOrientation::PositiveX);
	EXPECT_EQ(grid.cell(1, 0, 1).id, 1);
	EXPECT_EQ(grid.cell(1, 1, 1).id, 2);
}

TEST(Prefab, CarvesListedEmptyCellsAndLeavesUnlistedOnesUntouched)
{
	spk::VoxelGrid grid(spk::Vector3Int{3, 3, 3}, {5});

	// Only one cell is listed (as an explicit empty): its neighbors stay untouched.
	spk::Prefab prefab;
	prefab.addVoxel({0, 0, 0}, {});

	prefab.applyTo(grid, {1, 1, 1});
	EXPECT_TRUE(grid.cell(1, 1, 1).isEmpty());
	EXPECT_EQ(grid.cell(2, 1, 1).id, 5);
}

TEST(Prefab, SkipsVoxelsLandingOutsideTheGrid)
{
	spk::VoxelGrid grid(spk::Vector3Int{2, 2, 2});

	spk::Prefab prefab;
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

	spk::Prefab prefab;
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

	spk::Prefab prefab;
	prefab.addVoxel({0, 0, 0}, {test.cube});
	prefab.addVoxel({1, 0, 0}, {test.cube});

	// A quarter turn around the pivot (the origin): the 2x1x1 row swings toward -Z and
	// the cells' own orientation follows.
	map.applyPrefab(prefab, {4, 0, 4}, spk::VoxelOrientation::PositiveX);
	EXPECT_EQ(map.cell({4, 0, 4}).id, test.cube);
	EXPECT_EQ(map.cell({4, 0, 3}).id, test.cube);
	EXPECT_EQ(map.cell({4, 0, 4}).orientation, spk::VoxelOrientation::PositiveX);
	EXPECT_TRUE(map.cell({5, 0, 4}).isEmpty());
}

TEST(VoxelMap, AppliesEmbeddedLayersInTheChunkBelow)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {});

	spk::Prefab prefab;
	prefab.addVoxel({0, -1, 0}, {test.cube});
	prefab.addVoxel({0, 0, 0}, {test.cube});

	// The embedded layer crosses into the chunk below the destination; coverage must
	// generate it so the prefab lands whole.
	map.applyPrefab(prefab, {4, 0, 4});
	EXPECT_EQ(map.cell({4, -1, 4}).id, test.cube);
	EXPECT_EQ(map.cell({4, 0, 4}).id, test.cube);
}

TEST(VoxelMap, PrefabEmptyCellsCarveGeneratedContent)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [&](spk::VoxelChunk &p_chunk) {
		p_chunk.setCell({0, 0, 0}, {test.cube});
		p_chunk.setCell({1, 0, 0}, {test.cube});
	});

	spk::Prefab prefab;
	prefab.addVoxel({0, 0, 0}, {});

	map.applyPrefab(prefab, {0, 0, 0});
	EXPECT_TRUE(map.cell({0, 0, 0}).isEmpty());
	EXPECT_EQ(map.cell({1, 0, 0}).id, test.cube);
}

TEST(VoxelMap, ApplyingAnEmptyPrefabLoadsNothing)
{
	const TestRegistry test;
	spk::VoxelMap map(test.registry, [](spk::VoxelChunk &) {});

	const spk::Prefab prefab;
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

	spk::Prefab prefab;
	prefab.addVoxel({0, 0, 0}, {test.cube});
	map.applyPrefab(prefab, {15, 0, 0});

	EXPECT_TRUE(neighbor.renderer().needsSynchronization());
}
