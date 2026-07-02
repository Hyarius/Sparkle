#include "voxel/voxel_traversal.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	pg::VoxelRegistry loadVoxelRegistry()
	{
		pg::VoxelRegistry registry;
		registry.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
		return registry;
	}
}

TEST(VoxelTraversal, ResolvesAsymmetricHeightsForEveryOrientation)
{
	const pg::CardinalHeightSet local{
		.positiveX = 10,
		.negativeX = 20,
		.positiveZ = 30,
		.negativeZ = 40,
		.stationary = 50};

	const pg::CardinalHeightSet positiveZ =
		pg::resolveWorldHeights(local, pg::VoxelOrientation::PositiveZ);
	EXPECT_FLOAT_EQ(positiveZ.positiveX, 10);
	EXPECT_FLOAT_EQ(positiveZ.negativeX, 20);
	EXPECT_FLOAT_EQ(positiveZ.positiveZ, 30);
	EXPECT_FLOAT_EQ(positiveZ.negativeZ, 40);

	const pg::CardinalHeightSet positiveX =
		pg::resolveWorldHeights(local, pg::VoxelOrientation::PositiveX);
	EXPECT_FLOAT_EQ(positiveX.positiveX, 30);
	EXPECT_FLOAT_EQ(positiveX.negativeX, 40);
	EXPECT_FLOAT_EQ(positiveX.positiveZ, 20);
	EXPECT_FLOAT_EQ(positiveX.negativeZ, 10);

	const pg::CardinalHeightSet negativeX =
		pg::resolveWorldHeights(local, pg::VoxelOrientation::NegativeX);
	EXPECT_FLOAT_EQ(negativeX.positiveX, 40);
	EXPECT_FLOAT_EQ(negativeX.negativeX, 30);
	EXPECT_FLOAT_EQ(negativeX.positiveZ, 10);
	EXPECT_FLOAT_EQ(negativeX.negativeZ, 20);

	const pg::CardinalHeightSet negativeZ =
		pg::resolveWorldHeights(local, pg::VoxelOrientation::NegativeZ);
	EXPECT_FLOAT_EQ(negativeZ.positiveX, 20);
	EXPECT_FLOAT_EQ(negativeZ.negativeX, 10);
	EXPECT_FLOAT_EQ(negativeZ.positiveZ, 40);
	EXPECT_FLOAT_EQ(negativeZ.negativeZ, 30);
	EXPECT_FLOAT_EQ(negativeZ.stationary, 50);
}

TEST(VoxelTraversal, ClassifiesSolidPassableAndEmptyCells)
{
	const pg::VoxelRegistry registry = loadVoxelRegistry();
	const pg::VoxelCell stone{.id = registry.numericId("stone-block")};
	const pg::VoxelCell bush{.id = registry.numericId("bush")};
	const pg::VoxelCell empty;

	EXPECT_TRUE(pg::isSolid(stone, registry));
	EXPECT_FALSE(pg::isPassableSpace(stone, registry));
	EXPECT_FALSE(pg::isSolid(bush, registry));
	EXPECT_TRUE(pg::isPassableSpace(bush, registry));
	EXPECT_FALSE(pg::isSolid(empty, registry));
	EXPECT_TRUE(pg::isPassableSpace(empty, registry));
}

TEST(VoxelTraversal, StandableRequiresSolidBaseAndTwoPassableCells)
{
	const pg::VoxelRegistry registry = loadVoxelRegistry();
	const std::int32_t stone = registry.numericId("stone-block");
	const std::int32_t bush = registry.numericId("bush");
	pg::VoxelGrid grid({1, 4, 1});

	grid.cell(0, 0, 0).id = stone;
	EXPECT_TRUE(pg::isStandable(grid, {0, 0, 0}, registry));

	grid.cell(0, 1, 0).id = bush;
	EXPECT_TRUE(pg::isStandable(grid, {0, 0, 0}, registry));

	grid.cell(0, 2, 0).id = stone;
	EXPECT_FALSE(pg::isStandable(grid, {0, 0, 0}, registry));

	grid.cell(0, 2, 0) = {};
	grid.cell(0, 0, 0).id = bush;
	EXPECT_FALSE(pg::isStandable(grid, {0, 0, 0}, registry));
	EXPECT_FALSE(pg::isStandable(grid, {0, 3, 0}, registry));
}
