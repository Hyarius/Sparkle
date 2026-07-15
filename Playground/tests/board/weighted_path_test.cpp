#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"

#include "board/board_data.hpp"
#include "board/pathfinder.hpp"
#include "board/weighted_path.hpp"

#include <limits>
#include <vector>

namespace
{
	const pg::BattleUnitId Mover{1};
	const pg::BattleUnitId Ally{2};
	const pg::BattleUnitId Enemy{3};

	// A 3-wide, 4-deep field whose middle column is expensive: the two-step-shorter route straight
	// down it costs 7, while going around the outside costs 5.
	//
	//   z=0  # # #      the mover starts on (1, 0, 0)
	//   z=1  # 3 #
	//   z=2  # 3 #
	//   z=3  # # #      and is heading for (1, 0, 3)
	[[nodiscard]] pgtest::BoardFixture::Request expensiveMiddle()
	{
		return pgtest::BoardFixture::Request{
			.layers =
				{"###\n"
				 "#3#\n"
				 "#3#\n"
				 "###",
				 "...\n...\n...\n...",
				 "...\n...\n...\n..."},
			.deploymentDepth = 1};
	}

	// A 3x3 field with one cost-3 cell in the middle of the middle row. Crossing it costs exactly as
	// much as walking around it - but crossing takes two steps and walking around takes four.
	[[nodiscard]] pgtest::BoardFixture::Request equalCostShortcut()
	{
		return pgtest::BoardFixture::Request{
			.layers = {"###\n#3#\n###", "...\n...\n...", "...\n...\n..."},
			.deploymentDepth = 1};
	}

	[[nodiscard]] std::vector<pg::BoardCell> cellsOf(const std::optional<pg::WeightedPath> &p_path)
	{
		return p_path.has_value() ? p_path->cells : std::vector<pg::BoardCell>{};
	}

	// A flat 3x3 grid graph, built by hand so a case can control the order nodes and edges were
	// inserted in - which is exactly what must NOT reach the result.
	[[nodiscard]] pg::TraversalGraph flatGraph(bool p_reversed)
	{
		std::vector<pg::BoardCell> cells;
		for (int z = 0; z < 3; ++z)
		{
			for (int x = 0; x < 3; ++x)
			{
				cells.push_back({x, 0, z});
			}
		}
		if (p_reversed)
		{
			std::ranges::reverse(cells);
		}

		pg::TraversalGraph graph;
		for (const pg::BoardCell &cell : cells)
		{
			(void)graph.addNode(cell);
		}

		// Direction slots follow the builder's convention: +X, -X, +Z, -Z.
		const std::array<spk::Vector3Int, 4> offsets{
			spk::Vector3Int{1, 0, 0}, spk::Vector3Int{-1, 0, 0}, spk::Vector3Int{0, 0, 1}, spk::Vector3Int{0, 0, -1}};
		for (std::size_t index = 0; index < graph.size(); ++index)
		{
			for (std::size_t direction = 0; direction < offsets.size(); ++direction)
			{
				const std::optional<std::size_t> neighbor =
					graph.indexOf(graph.node(index).position + offsets[direction]);
				if (neighbor.has_value())
				{
					graph.connect(index, direction, *neighbor);
				}
			}
		}
		return graph;
	}
}

TEST(WeightedPathTest, TheCheapestRouteIsTakenEvenWhenItIsTheLongOne)
{
	pgtest::BoardFixture fixture(expensiveMiddle());
	const pg::BoardData &board = fixture.board();
	const pg::TraversalCostQuery query = pg::boardMovementQuery(board, Mover);

	const std::optional<pg::WeightedPath> path =
		pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, 10, query);
	ASSERT_TRUE(path.has_value());

	// Around the outside: five cost-1 cells, rather than the three-step route across two cost-3 ones
	// (3 + 3 + 1 = 7). The source is in the sequence and is not paid for.
	EXPECT_EQ(
		cellsOf(path),
		(std::vector<pg::BoardCell>{{1, 0, 0}, {0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 0, 3}, {1, 0, 3}}));
	EXPECT_EQ(path->totalCost, 5);

	// The two five-cost routes (around x = 0 and around x = 2) enter the same number of cells, so the
	// tie falls to the lexicographically smaller complete sequence - which is the x = 0 one.
	EXPECT_EQ(path->cells[1], (pg::BoardCell{0, 0, 0}));
}

TEST(WeightedPathTest, TheBudgetIsCheckedOnTheExactSummedCost)
{
	pgtest::BoardFixture fixture(expensiveMiddle());
	const pg::BoardData &board = fixture.board();
	const pg::TraversalCostQuery query = pg::boardMovementQuery(board, Mover);

	EXPECT_TRUE(pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, 5, query).has_value());
	EXPECT_FALSE(pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, 4, query).has_value())
		<< "one point short of the cheapest route is no route at all";
	EXPECT_FALSE(pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, -1, query).has_value());

	// The destination's own cost is paid; the source's is not.
	const std::optional<pg::WeightedPath> single =
		pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 1}, 3, query);
	ASSERT_TRUE(single.has_value());
	EXPECT_EQ(single->totalCost, 3) << "entering the cost-3 cell, and nothing for standing where it stood";

	// from == to is the identity route: the mover is admitted onto its own cell for free.
	const std::optional<pg::WeightedPath> nowhere =
		pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 0}, 0, query);
	ASSERT_TRUE(nowhere.has_value());
	EXPECT_EQ(cellsOf(nowhere), (std::vector<pg::BoardCell>{{1, 0, 0}}));
	EXPECT_EQ(nowhere->totalCost, 0);

	// A cell that is not a node is not a destination.
	EXPECT_FALSE(pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 1, 0}, 10, query).has_value());
}

TEST(WeightedPathTest, EveryOccupiedCellBlocksAndTheMoverNeverBlocksItself)
{
	pgtest::BoardFixture fixture(expensiveMiddle());
	pg::BoardData &board = fixture.board();
	const pg::BoardMutation mutation = fixture.mutation();
	const pg::TraversalCostQuery query = pg::boardMovementQuery(board, Mover);

	// The mover stands on its own source cell, and that must not stop it from leaving.
	ASSERT_TRUE(mutation.placeUnit(Mover, {1, 0, 0}).changed);
	ASSERT_TRUE(pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, 10, query).has_value());

	// An ally in the cheap corridor blocks it exactly as an enemy would: v1 has no pass-through. The
	// route flips to the mirror side, still for 5.
	ASSERT_TRUE(mutation.placeUnit(Ally, {0, 0, 1}).changed);
	const std::optional<pg::WeightedPath> around =
		pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, 10, query);
	ASSERT_TRUE(around.has_value());
	EXPECT_EQ(around->cells[1], (pg::BoardCell{2, 0, 0}));
	EXPECT_EQ(around->totalCost, 5);

	// With both corridors held, only the expensive middle is left.
	ASSERT_TRUE(mutation.placeUnit(Enemy, {2, 0, 1}).changed);
	const std::optional<pg::WeightedPath> across =
		pg::findWeightedPath(board.navigation(), {1, 0, 0}, {1, 0, 3}, 10, query);
	ASSERT_TRUE(across.has_value());
	EXPECT_EQ(across->totalCost, 7);
	EXPECT_EQ(cellsOf(across), (std::vector<pg::BoardCell>{{1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {1, 0, 3}}));

	// An occupied cell is not a destination either.
	EXPECT_FALSE(pg::findWeightedPath(board.navigation(), {1, 0, 0}, {0, 0, 1}, 10, query).has_value());
}

TEST(WeightedPathTest, EqualCostPrefersFewerEnteredCells)
{
	pgtest::BoardFixture fixture(equalCostShortcut());
	const pg::BoardData &board = fixture.board();
	const pg::TraversalCostQuery query = pg::boardMovementQuery(board, Mover);

	// Straight across the cost-3 cell: 3 + 1 = 4, entering two cells. Around it: four cost-1 cells,
	// also 4. Same MP, so the shorter walk wins - even though the search settles it later.
	const std::optional<pg::WeightedPath> path =
		pg::findWeightedPath(board.navigation(), {0, 0, 1}, {2, 0, 1}, 10, query);
	ASSERT_TRUE(path.has_value());
	EXPECT_EQ(path->totalCost, 4);
	EXPECT_EQ(cellsOf(path), (std::vector<pg::BoardCell>{{0, 0, 1}, {1, 0, 1}, {2, 0, 1}}));
}

TEST(WeightedPathTest, TheResultDoesNotDependOnGraphInsertionOrder)
{
	// Same cells, same edges, opposite insertion order. A path that changed here would be a path
	// decided by std::priority_queue accidents rather than by the rules.
	const pg::TraversalGraph forward = flatGraph(false);
	const pg::TraversalGraph reversed = flatGraph(true);
	const pg::TraversalCostQuery query = pg::uniformCostQuery(1);

	const std::optional<pg::WeightedPath> first =
		pg::findWeightedPath(forward, {0, 0, 0}, {2, 0, 2}, 10, query);
	const std::optional<pg::WeightedPath> second =
		pg::findWeightedPath(reversed, {0, 0, 0}, {2, 0, 2}, 10, query);
	ASSERT_TRUE(first.has_value());
	ASSERT_TRUE(second.has_value());
	EXPECT_EQ(*first, *second);
	EXPECT_EQ(first->totalCost, 4);

	// And it is the canonical one: of all the four-step routes, the lexicographically smallest
	// complete sequence under BoardCellLess.
	EXPECT_EQ(
		cellsOf(first),
		(std::vector<pg::BoardCell>{{0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}}));

	EXPECT_EQ(pg::floodWeighted(forward, {0, 0, 0}, 2, query), pg::floodWeighted(reversed, {0, 0, 0}, 2, query));
}

TEST(WeightedPathTest, TheFloodIsCanonicalAndIncludesItsSource)
{
	pgtest::BoardFixture fixture(expensiveMiddle());
	const pg::BoardData &board = fixture.board();
	const pg::TraversalCostQuery query = pg::boardMovementQuery(board, Mover);

	const std::map<pg::BoardCell, int, pg::BoardCellLess> reachable =
		pg::floodWeighted(board.navigation(), {1, 0, 0}, 2, query);

	// The source at 0, the two neighbours at 1, the cells behind them at 2 - and NOT the cost-3 cell
	// straight ahead, which is over the budget.
	const std::map<pg::BoardCell, int, pg::BoardCellLess> expected{
		{{0, 0, 0}, 1}, {{0, 0, 1}, 2}, {{1, 0, 0}, 0}, {{2, 0, 0}, 1}, {{2, 0, 1}, 2}};
	EXPECT_EQ(reachable, expected);

	EXPECT_TRUE(pg::floodWeighted(board.navigation(), {1, 0, 0}, -1, query).empty());
	EXPECT_TRUE(pg::floodWeighted(board.navigation(), {9, 9, 9}, 5, query).empty());
}

TEST(WeightedPathTest, CheckedCostArithmeticRefusesToWrap)
{
	const pg::TraversalGraph graph = flatGraph(false);
	const pg::TraversalCostQuery enormous = pg::uniformCostQuery(std::numeric_limits<int>::max());

	// One step of this costs the entire budget; a second would wrap the sum, so it is refused rather
	// than folded into a cheap negative route.
	const std::optional<pg::WeightedPath> single =
		pg::findWeightedPath(graph, {0, 0, 0}, {1, 0, 0}, std::numeric_limits<int>::max(), enormous);
	ASSERT_TRUE(single.has_value());
	EXPECT_EQ(single->totalCost, std::numeric_limits<int>::max());

	EXPECT_FALSE(
		pg::findWeightedPath(graph, {0, 0, 0}, {2, 0, 0}, std::numeric_limits<int>::max(), enormous).has_value());

	// A zero or negative enter cost would make a cycle free, so it is a broken query, not a fast one.
	EXPECT_THROW(auto value = pg::uniformCostQuery(0), std::invalid_argument);
	const pg::TraversalCostQuery negative{
		.enterCost = [](const spk::Vector3Int &) { return std::optional<int>(-1); }};
	EXPECT_THROW(auto value = pg::findWeightedPath(graph, {0, 0, 0}, {2, 0, 0}, 10, negative), std::invalid_argument);
}

TEST(WeightedPathTest, TheLegacyExplorationPathIsStillValidAndNowDeterministic)
{
	const pg::TraversalGraph forward = flatGraph(false);
	const pg::TraversalGraph reversed = flatGraph(true);

	const std::optional<std::vector<spk::Vector3Int>> path =
		pg::Pathfinder::findPath(forward, {0, 0, 0}, {2, 0, 2});
	ASSERT_TRUE(path.has_value());
	EXPECT_EQ(path->front(), spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(path->back(), spk::Vector3Int(2, 0, 2));
	EXPECT_EQ(path->size(), 5U) << "four unit steps, plus the source";
	EXPECT_EQ(*path, *pg::Pathfinder::findPath(reversed, {0, 0, 0}, {2, 0, 2}));

	// The float budget floors: 3.9 buys three steps, which does not reach a four-step goal.
	EXPECT_TRUE(pg::Pathfinder::findPath(forward, {0, 0, 0}, {2, 0, 2}, 4.0f).has_value());
	EXPECT_FALSE(pg::Pathfinder::findPath(forward, {0, 0, 0}, {2, 0, 2}, 3.9f).has_value());
	EXPECT_FALSE(pg::Pathfinder::findPath(forward, {0, 0, 0}, {9, 0, 9}).has_value());

	const std::map<spk::Vector3Int, float, pg::CellPositionLess> reachable =
		pg::Pathfinder::floodReachable(forward, {0, 0, 0}, 1.0f);
	EXPECT_EQ(reachable.size(), 3U) << "the source and its two neighbours";
	EXPECT_FLOAT_EQ(reachable.at({0, 0, 0}), 0.0f);
	EXPECT_FLOAT_EQ(reachable.at({1, 0, 0}), 1.0f);
	EXPECT_TRUE(pg::Pathfinder::floodReachable(forward, {0, 0, 0}, -1.0f).empty());
}
