#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "structures/container/spk_grid_2d.hpp"
#include "structures/math/spk_cellular_automata.hpp"

namespace
{
	spk::Grid2D<int> makeGrid(std::size_t p_width, std::size_t p_height, std::vector<int> p_cells)
	{
		spk::Grid2D<int> grid(spk::Vector2UInt(static_cast<std::uint32_t>(p_width), static_cast<std::uint32_t>(p_height)));

		for (std::size_t y = 0; y < p_height; ++y)
		{
			for (std::size_t x = 0; x < p_width; ++x)
			{
				grid(x, y) = p_cells[y * p_width + x];
			}
		}

		return grid;
	}

	bool isAlive(const int &p_cell)
	{
		return p_cell == 1;
	}
}

TEST(CellularAutomataTest, IsNotConstructible)
{
	EXPECT_FALSE(std::is_default_constructible_v<spk::CellularAutomata>);
}

TEST(CellularAutomataTest, NextBirthsCellSurroundedByStateA)
{
	// Center cell is state B but every neighbour is state A; with the default
	// rule (birthLimit = 4) the count of 8 exceeds the limit, so it becomes A.
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		1, 1, 1,
		1, 0, 1,
		1, 1, 1});

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0);

	EXPECT_EQ(result(1, 1), 1);
}

TEST(CellularAutomataTest, NextKillsStateACellWithTooFewNeighbours)
{
	// Default rule deathLimit = 3. The center A cell has only 2 A neighbours
	// (corners treated as state A by default edge policy add more, so isolate it).
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		0, 0, 0,
		0, 1, 0,
		0, 0, 0});

	spk::CellularAutomata::Rule rule;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::TreatOutsideAsStateB;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	EXPECT_EQ(result(1, 1), 0);
}

TEST(CellularAutomataTest, NextKeepsStateACellWithEnoughNeighbours)
{
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		1, 1, 1,
		1, 1, 0,
		0, 0, 0});

	spk::CellularAutomata::Rule rule;
	rule.deathLimit = 3;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::TreatOutsideAsStateB;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	// Center has 4 living neighbours (>= deathLimit) so it survives.
	EXPECT_EQ(result(1, 1), 1);
}

TEST(CellularAutomataTest, NextKeepsStateBCellWithTooFewNeighbours)
{
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		0, 0, 0,
		0, 0, 0,
		0, 0, 0});

	spk::CellularAutomata::Rule rule;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::TreatOutsideAsStateB;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	EXPECT_EQ(result(1, 1), 0);
}

TEST(CellularAutomataTest, EdgePolicyTreatOutsideAsStateACountsBorderAsAlive)
{
	// A single border cell: with TreatOutsideAsStateA all 5 out-of-bounds
	// neighbours count as A, so a B corner gets birthed.
	const spk::Grid2D<int> grid = makeGrid(2, 2, {
		0, 0,
		0, 0});

	spk::CellularAutomata::Rule rule;
	rule.birthLimit = 4;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::TreatOutsideAsStateA;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	// Corner (0,0) has 5 outside neighbours (all A) -> birthed.
	EXPECT_EQ(result(0, 0), 1);
}

TEST(CellularAutomataTest, EdgePolicyTreatOutsideAsStateBCountsBorderAsDead)
{
	const spk::Grid2D<int> grid = makeGrid(2, 2, {
		0, 0,
		0, 0});

	spk::CellularAutomata::Rule rule;
	rule.birthLimit = 4;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::TreatOutsideAsStateB;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	EXPECT_EQ(result(0, 0), 0);
}

TEST(CellularAutomataTest, EdgePolicyWrapCountsNeighboursAcrossBorders)
{
	// On a 3x3 grid all cells alive, Wrap means every cell has 8 living
	// neighbours regardless of position, so all survive with deathLimit 3.
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		1, 1, 1,
		1, 1, 1,
		1, 1, 1});

	spk::CellularAutomata::Rule rule;
	rule.deathLimit = 3;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::Wrap;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	for (std::size_t y = 0; y < 3; ++y)
	{
		for (std::size_t x = 0; x < 3; ++x)
		{
			EXPECT_EQ(result(x, y), 1) << "at (" << x << ", " << y << ")";
		}
	}
}

TEST(CellularAutomataTest, WrapKillsIsolatedCellByWrappingToEmptyNeighbours)
{
	// A lone alive cell on a wrapped grid wraps onto dead neighbours and dies.
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		0, 0, 0,
		0, 1, 0,
		0, 0, 0});

	spk::CellularAutomata::Rule rule;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::Wrap;

	const spk::Grid2D<int> result = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	EXPECT_EQ(result(1, 1), 0);
}

TEST(CellularAutomataTest, NextDefaultRuleOverloadMatchesExplicitDefaultRule)
{
	const spk::Grid2D<int> grid = makeGrid(3, 3, {
		1, 0, 1,
		0, 1, 0,
		1, 0, 1});

	const spk::Grid2D<int> implicitRule = spk::CellularAutomata::next(grid, isAlive, 1, 0);
	const spk::Grid2D<int> explicitRule =
		spk::CellularAutomata::next(grid, isAlive, 1, 0, spk::CellularAutomata::Rule{});

	ASSERT_EQ(implicitRule.size(), explicitRule.size());

	for (std::size_t y = 0; y < 3; ++y)
	{
		for (std::size_t x = 0; x < 3; ++x)
		{
			EXPECT_EQ(implicitRule(x, y), explicitRule(x, y));
		}
	}
}

TEST(CellularAutomataTest, ApplyMutatesGridInPlace)
{
	spk::Grid2D<int> grid = makeGrid(3, 3, {
		1, 1, 1,
		1, 0, 1,
		1, 1, 1});

	const spk::Grid2D<int> expected = spk::CellularAutomata::next(grid, isAlive, 1, 0);

	spk::CellularAutomata::apply(grid, isAlive, 1, 0);

	for (std::size_t y = 0; y < 3; ++y)
	{
		for (std::size_t x = 0; x < 3; ++x)
		{
			EXPECT_EQ(grid(x, y), expected(x, y));
		}
	}
}

TEST(CellularAutomataTest, ApplyWithRuleMutatesGridInPlace)
{
	spk::Grid2D<int> grid = makeGrid(3, 3, {
		1, 1, 1,
		1, 1, 1,
		1, 1, 1});

	spk::CellularAutomata::Rule rule;
	rule.deathLimit = 3;
	rule.edgePolicy = spk::CellularAutomata::EdgePolicy::Wrap;

	const spk::Grid2D<int> expected = spk::CellularAutomata::next(grid, isAlive, 1, 0, rule);

	spk::CellularAutomata::apply(grid, isAlive, 1, 0, rule);

	for (std::size_t y = 0; y < 3; ++y)
	{
		for (std::size_t x = 0; x < 3; ++x)
		{
			EXPECT_EQ(grid(x, y), expected(x, y));
		}
	}
}
