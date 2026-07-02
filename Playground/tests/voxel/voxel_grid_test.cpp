#include "voxel/voxel_data.hpp"
#include "voxel/voxel_grid.hpp"

#include <gtest/gtest.h>

TEST(VoxelData, TagLookupIsTrimmedAndCaseInsensitive)
{
	const pg::VoxelData data{.tags = {" Ground ", "BuSh"}};
	EXPECT_TRUE(data.hasTag("ground"));
	EXPECT_TRUE(data.hasTag("  BUSH  "));
	EXPECT_FALSE(data.hasTag(""));
	EXPECT_FALSE(data.hasTag("stone"));
}

TEST(VoxelGrid, ProvidesDenseBoundsCheckedStorage)
{
	pg::VoxelGrid grid({2, 3, 4});
	EXPECT_EQ(grid.cells().size(), 24);
	EXPECT_TRUE(grid.isWithinBounds(0, 0, 0));
	EXPECT_TRUE(grid.isWithinBounds(1, 2, 3));
	EXPECT_FALSE(grid.isWithinBounds(-1, 0, 0));
	EXPECT_FALSE(grid.isWithinBounds(2, 0, 0));

	grid.cell(1, 2, 3).id = 42;
	EXPECT_EQ(grid.cell({1, 2, 3}).id, 42);
	EXPECT_THROW((void)grid.cell(2, 0, 0), std::out_of_range);
	EXPECT_THROW((void)grid.cell(0, 3, 0), std::out_of_range);
}

TEST(VoxelGrid, RejectsNegativeDimensions)
{
	EXPECT_THROW((void)pg::VoxelGrid({1, -1, 1}), std::invalid_argument);
}
