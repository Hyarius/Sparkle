#include "board/board_builder.hpp"
#include "board/board_raycaster.hpp"
#include "board/line_of_sight.hpp"
#include "support/board_fixture.hpp"

#include <gtest/gtest.h>

TEST(LineOfSight, WallBlocksButTransparentBushDoesNot)
{
	pg::test::BoardFixture wall({"#x#"});
	EXPECT_FALSE(pg::LineOfSight::hasLine(wall.board().terrain().cells(), wall.cell(0, 0), wall.cell(2, 0)));

	pg::test::BoardFixture bush({"#b#"});
	EXPECT_TRUE(pg::LineOfSight::hasLine(bush.board().terrain().cells(), bush.cell(0, 0), bush.cell(2, 0)));
}

TEST(LineOfSight, HandlesElevationAndAlwaysConnectsAdjacentCells)
{
	pg::VoxelGrid grid({5, 6, 1});
	const auto ground = pg::VoxelCell{pg::test::BoardFixture::registry().numericId("stone-block")};
	for (int x = 0; x < 5; ++x)
	{
		grid.cell(x, 0, 0) = ground;
	}
	grid.cell(0, 2, 0) = ground;
	pg::GridCellSource source(grid, pg::test::BoardFixture::registry());

	EXPECT_TRUE(pg::LineOfSight::hasLine(source, {0, 2, 0}, {4, 0, 0}));
	EXPECT_TRUE(pg::LineOfSight::hasLine(source, {0, 2, 0}, {1, 0, 0}));
}

TEST(BoardRaycaster, ResolvesTopStandableCellInHitColumn)
{
	pg::VoxelGrid grid({1, 6, 1});
	const auto ground = pg::VoxelCell{pg::test::BoardFixture::registry().numericId("stone-block")};
	grid.cell(0, 0, 0) = ground;
	grid.cell(0, 2, 0) = ground;
	const pg::BoardData board = pg::BoardBuilder::fromGrid(grid, pg::test::BoardFixture::registry());

	const auto selected = pg::BoardRaycaster::resolveMouseCell(
		board, {.origin = {0.5f, 8.0f, 0.5f}, .direction = {0, -1, 0}}, 20.0f);
	ASSERT_TRUE(selected.has_value());
	EXPECT_EQ(*selected, spk::Vector3Int(0, 2, 0));
}
