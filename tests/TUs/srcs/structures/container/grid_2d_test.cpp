#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

#include "structures/container/spk_grid_2d.hpp"

TEST(Grid2DTest, DefaultConstructorIsEmpty)
{
	const spk::Grid2D<int> grid;

	EXPECT_EQ(grid.size(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(grid.width(), 0u);
	EXPECT_EQ(grid.height(), 0u);
	EXPECT_TRUE(grid.empty());
	EXPECT_TRUE(grid.data().empty());
}

TEST(Grid2DTest, SizeConstructorFillsWithDefault)
{
	const spk::Grid2D<int> grid(spk::Vector2UInt(3, 2));

	EXPECT_EQ(grid.width(), 3u);
	EXPECT_EQ(grid.height(), 2u);
	EXPECT_FALSE(grid.empty());
	EXPECT_EQ(grid.data().size(), 6u);

	for (std::size_t y = 0; y < grid.height(); ++y)
	{
		for (std::size_t x = 0; x < grid.width(); ++x)
		{
			EXPECT_EQ(grid(x, y), 0);
		}
	}
}

TEST(Grid2DTest, SizeConstructorUsesProvidedFill)
{
	const spk::Grid2D<int> grid(spk::Vector2UInt(2, 2), 7);

	for (const int value : grid.data())
	{
		EXPECT_EQ(value, 7);
	}
}

TEST(Grid2DTest, ContainsDistinguishesInsideAndOutside)
{
	const spk::Grid2D<int> grid(spk::Vector2UInt(3, 2));

	EXPECT_TRUE(grid.contains(0, 0));
	EXPECT_TRUE(grid.contains(2, 1));
	EXPECT_FALSE(grid.contains(3, 0)); // x out of range
	EXPECT_FALSE(grid.contains(0, 2)); // y out of range
}

TEST(Grid2DTest, IndexOfComputesRowMajorIndex)
{
	const spk::Grid2D<int> grid(spk::Vector2UInt(4, 3));

	EXPECT_EQ(grid.indexOf(0, 0), 0u);
	EXPECT_EQ(grid.indexOf(1, 0), 1u);
	EXPECT_EQ(grid.indexOf(0, 1), 4u);
	EXPECT_EQ(grid.indexOf(3, 2), 11u);
}

TEST(Grid2DTest, IndexOfThrowsWhenOutOfRange)
{
	const spk::Grid2D<int> grid(spk::Vector2UInt(2, 2));

	EXPECT_THROW(auto value = grid.indexOf(2, 0), std::out_of_range);
	EXPECT_THROW(auto value = grid.indexOf(0, 2), std::out_of_range);
}

TEST(Grid2DTest, AtReadsAndWrites)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(2, 2));

	grid.at(1, 0) = 5;
	grid.at(0, 1) = 9;

	EXPECT_EQ(grid.at(1, 0), 5);
	EXPECT_EQ(grid.at(0, 1), 9);

	const spk::Grid2D<int> &constGrid = grid;
	EXPECT_EQ(constGrid.at(1, 0), 5);
	EXPECT_EQ(constGrid.at(0, 1), 9);
}

TEST(Grid2DTest, AtThrowsWhenOutOfRange)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(2, 2));
	const spk::Grid2D<int> &constGrid = grid;

	EXPECT_THROW(auto& value = grid.at(2, 2), std::out_of_range);
	EXPECT_THROW(const auto& value = constGrid.at(2, 2), std::out_of_range);
}

TEST(Grid2DTest, CallOperatorReadsAndWrites)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(2, 2));

	grid(1, 1) = 42;

	EXPECT_EQ(grid(1, 1), 42);

	const spk::Grid2D<int> &constGrid = grid;
	EXPECT_EQ(constGrid(1, 1), 42);
}

TEST(Grid2DTest, SubscriptOperatorReadsAndWrites)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(2, 2));

	grid[0, 1] = 11;

	EXPECT_EQ((grid[0, 1]), 11);

	const spk::Grid2D<int> &constGrid = grid;
	EXPECT_EQ((constGrid[0, 1]), 11);
}

TEST(Grid2DTest, FillOverwritesEveryCell)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(3, 3), 1);

	grid.fill(4);

	for (const int value : grid.data())
	{
		EXPECT_EQ(value, 4);
	}
}

TEST(Grid2DTest, ResizeReplacesContentsAndSize)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(2, 2), 1);

	grid.resize(spk::Vector2UInt(3, 1), 8);

	EXPECT_EQ(grid.size(), spk::Vector2UInt(3, 1));
	EXPECT_EQ(grid.data().size(), 3u);

	for (const int value : grid.data())
	{
		EXPECT_EQ(value, 8);
	}
}

TEST(Grid2DTest, ResizeToEmptyClearsStorage)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(2, 2), 1);

	grid.resize(spk::Vector2UInt(0, 0));

	EXPECT_TRUE(grid.empty());
	EXPECT_EQ(grid.width(), 0u);
	EXPECT_EQ(grid.height(), 0u);
}

TEST(Grid2DTest, SupportsNonTrivialValueTypes)
{
	spk::Grid2D<std::string> grid(spk::Vector2UInt(2, 1), "x");

	grid(0, 0) = "hello";

	EXPECT_EQ(grid(0, 0), "hello");
	EXPECT_EQ(grid(1, 0), "x");
}

TEST(Grid2DTest, ResizeThrowsWhenElementCountExceedsMaxSize)
{
	// width * height (~1.8e19) exceeds std::vector<int>::max_size() (~4.6e18),
	// so the element-count guard rejects the request before allocating.
	spk::Grid2D<int> grid;

	const std::uint32_t huge = std::numeric_limits<std::uint32_t>::max();

	EXPECT_THROW(grid.resize(spk::Vector2UInt(huge, huge)), std::length_error);
}
