#include "support/board_fixture.hpp"

#include <gtest/gtest.h>

TEST(BoardFixture, LegendBuildsExpectedStandableColumns)
{
	pg::test::BoardFixture fixture({"#/\\<>s_bx."});

	for (int x = 0; x < 9; ++x)
	{
		EXPECT_TRUE(fixture.board().isStandable(fixture.cell(x, 0))) << "symbol index " << x;
	}
	EXPECT_THROW((void)fixture.cell(9, 0), std::out_of_range);
	EXPECT_EQ(fixture.cell(7, 0), spk::Vector3Int(7, 0, 0));
	EXPECT_EQ(fixture.cell(8, 0), spk::Vector3Int(8, 2, 0));
}

TEST(BoardFixture, StoresAndResolvesNamedCells)
{
	pg::test::BoardFixture fixture({"###"});
	fixture.name("left", fixture.cell(0, 0));
	fixture.name("right", fixture.cell(2, 0));

	EXPECT_EQ(fixture.cell("left"), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(fixture.cell("right"), spk::Vector3Int(2, 0, 0));
	EXPECT_THROW((void)fixture.cell("missing"), std::out_of_range);
}

TEST(BoardFixture, ParsesExplicitLayersBottomUp)
{
	pg::test::BoardFixture fixture(std::vector<std::vector<std::string>>{{"###"}, {"..#"}});

	EXPECT_EQ(fixture.cell(0, 0), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(fixture.cell(2, 0), spk::Vector3Int(2, 1, 0));
}
