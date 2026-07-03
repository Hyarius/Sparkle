#include "battle/rules/battle_geometry.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <utility>

namespace
{
	using pg::AreaShape;
	using pg::RangeShape;

	[[nodiscard]] std::set<std::pair<int, int>> offsetSet(
		AreaShape p_shape, int p_value, spk::Vector2Int p_direction)
	{
		std::set<std::pair<int, int>> result;
		for (const spk::Vector2Int &offset : pg::BattleGeometry::areaOffsets(p_shape, p_value, p_direction))
		{
			result.emplace(offset.x, offset.y);
		}
		return result;
	}
}

TEST(BattleGeometry, CircleRangeIsAManhattanRing)
{
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Circle, {0, 0}, 1, 2)); // below min
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Circle, {1, 0}, 1, 2));
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Circle, {1, 1}, 1, 2));
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Circle, {0, 2}, 1, 2));
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Circle, {2, 1}, 1, 2)); // d=3 > max
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Circle, {-2, 0}, 1, 2)); // symmetric
}

TEST(BattleGeometry, LineRangeOnlyFollowsTheFourAxes)
{
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Line, {3, 0}, 1, 3));
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Line, {0, -2}, 1, 3));
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Line, {2, 2}, 1, 3)); // diagonal
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Line, {0, 0}, 0, 3)); // origin excluded
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Line, {4, 0}, 1, 3)); // beyond max
}

TEST(BattleGeometry, DiagonalRangeOnlyFollowsTheFourDiagonals)
{
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Diagonal, {2, 2}, 1, 3));
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Diagonal, {-1, 1}, 1, 3));
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Diagonal, {2, 1}, 1, 3)); // not equal
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Diagonal, {3, 0}, 1, 3)); // axis
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Diagonal, {0, 0}, 0, 3)); // origin
}

TEST(BattleGeometry, SelfRangeIsOnlyTheCasterCell)
{
	EXPECT_TRUE(pg::BattleGeometry::rangeContains(RangeShape::Self, {0, 0}, 0, 5));
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Self, {1, 0}, 0, 5));
	EXPECT_FALSE(pg::BattleGeometry::rangeContains(RangeShape::Self, {0, 1}, 0, 5));
}

TEST(BattleGeometry, AreaValueZeroIsAlwaysASingleCell)
{
	for (AreaShape shape : {AreaShape::Square, AreaShape::Cross, AreaShape::Circle, AreaShape::Line})
	{
		const auto cells = offsetSet(shape, 0, {1, 0});
		EXPECT_EQ(cells.size(), 1u);
		EXPECT_TRUE(cells.contains({0, 0}));
	}
}

TEST(BattleGeometry, SquareAndCircleAndCrossAoEStruthTables)
{
	// square value 1 == Chebyshev disk (the full 3x3 block).
	EXPECT_EQ(offsetSet(AreaShape::Square, 1, {0, 0}).size(), 9u);
	EXPECT_TRUE(offsetSet(AreaShape::Square, 1, {0, 0}).contains({1, 1}));

	// circle value 1 == Manhattan disk (5 cells, corners dropped).
	const auto circle = offsetSet(AreaShape::Circle, 1, {0, 0});
	EXPECT_EQ(circle.size(), 5u);
	EXPECT_FALSE(circle.contains({1, 1}));
	EXPECT_TRUE(circle.contains({-1, 0}));

	// cross value 2 == the plus arms only.
	const auto cross = offsetSet(AreaShape::Cross, 2, {0, 0});
	EXPECT_EQ(cross.size(), 9u); // center + 4 arms * 2
	EXPECT_TRUE(cross.contains({0, 2}));
	EXPECT_TRUE(cross.contains({-2, 0}));
	EXPECT_FALSE(cross.contains({1, 1}));
}

TEST(BattleGeometry, LineAoEFollowsCasterToAnchorDirection)
{
	// Anchor east of the caster -> the line extends further east.
	const auto east = offsetSet(AreaShape::Line, 2, {3, 0});
	EXPECT_EQ(east, (std::set<std::pair<int, int>>{{0, 0}, {1, 0}, {2, 0}}));

	// Anchor north (dominant z) -> the line extends along z.
	const auto north = offsetSet(AreaShape::Line, 1, {0, -4});
	EXPECT_EQ(north, (std::set<std::pair<int, int>>{{0, 0}, {0, -1}}));
}
