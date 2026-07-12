#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/voxel/spk_voxel_grid.hpp"

TEST(VoxelGrid, DefaultConstructedGridIsEmpty)
{
	const spk::VoxelGrid grid;

	EXPECT_EQ(grid.size(), spk::Vector3Int(0, 0, 0));
	EXPECT_TRUE(grid.cells().empty());
	EXPECT_FALSE(grid.isWithinBounds(0, 0, 0));
}

TEST(VoxelGrid, SizedGridStartsWithTheFillCell)
{
	const spk::VoxelCell fill{spk::VoxelRuntimeId{4}, spk::VoxelOrientation::NegativeX, spk::VoxelFlip::NegativeY};
	const spk::VoxelGrid grid({2, 3, 4}, fill);

	EXPECT_EQ(grid.cells().size(), 24u);
	EXPECT_EQ(grid.cell(1, 2, 3), fill);
	EXPECT_FALSE(grid.cell(0, 0, 0).isEmpty());
}

TEST(VoxelGrid, CellsDefaultToEmpty)
{
	const spk::VoxelGrid grid({2, 2, 2});

	EXPECT_TRUE(grid.cell(1, 1, 1).isEmpty());
}

TEST(VoxelGrid, BoundsAreChecked)
{
	spk::VoxelGrid grid({2, 2, 2});

	EXPECT_TRUE(grid.isWithinBounds(1, 1, 1));
	EXPECT_FALSE(grid.isWithinBounds(-1, 0, 0));
	EXPECT_FALSE(grid.isWithinBounds(0, 2, 0));
	EXPECT_THROW((void)grid.cell(2, 0, 0), std::out_of_range);
	EXPECT_THROW((void)grid.cell({0, 0, -1}), std::out_of_range);
}

TEST(VoxelGrid, NegativeSizeIsRejected)
{
	EXPECT_THROW(spk::VoxelGrid({-1, 2, 2}), std::invalid_argument);
}

TEST(VoxelGrid, WrittenCellsAreReadBack)
{
	spk::VoxelGrid grid({4, 4, 4});
	grid.cell(1, 2, 3) = {spk::VoxelRuntimeId{7}};

	EXPECT_EQ(grid.cell({1, 2, 3}).id, spk::VoxelRuntimeId{7});
	EXPECT_FALSE(grid.cell(1, 2, 3).isEmpty());
}
