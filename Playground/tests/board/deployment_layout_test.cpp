#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"

#include "board/board_data.hpp"
#include "board/deployment_layout.hpp"

#include <algorithm>
#include <vector>

namespace
{
	[[nodiscard]] std::vector<pg::BoardCell> zone(const pg::BoardData &p_board, pg::BattleSide p_side)
	{
		const std::span<const pg::BoardCell> cells = p_board.deployment().cells(p_side);
		return {cells.begin(), cells.end()};
	}
}

TEST(DeploymentLayoutTest, ThePlayerDeploysOnTheEdgeItWalkedInFrom)
{
	// The approach is the player's last move into the encounter, so it stands on the edge that move
	// came from and the enemy waits on the far one. All four horizontal approaches, one board.
	struct Case
	{
		pg::VoxelOrientation approach;
		int playerRow;
		int enemyRow;
		bool axisIsZ;
	};
	const std::array<Case, 4> cases{
		Case{pg::VoxelOrientation::PositiveZ, 0, 4, true},
		Case{pg::VoxelOrientation::NegativeZ, 4, 0, true},
		Case{pg::VoxelOrientation::PositiveX, 0, 4, false},
		Case{pg::VoxelOrientation::NegativeX, 4, 0, false}};

	for (const Case &current : cases)
	{
		pgtest::BoardFixture::Request request = pgtest::BoardFixture::flat(5, 5);
		request.playerApproach = current.approach;
		request.deploymentDepth = 1;
		pgtest::BoardFixture fixture(request);
		const pg::DeploymentLayout &deployment = fixture.board().deployment();

		EXPECT_EQ(deployment.isDeploymentAxisZ(), current.axisIsZ);
		EXPECT_EQ(deployment.playerCells.size(), 5U) << "one row of five";
		EXPECT_EQ(deployment.enemyCells.size(), 5U);

		for (const pg::BoardCell &cell : deployment.playerCells)
		{
			EXPECT_EQ(deployment.deploymentAxisCoordinate(cell), current.playerRow);
			EXPECT_TRUE(deployment.contains(pg::BattleSide::Player, cell));
			EXPECT_FALSE(deployment.contains(pg::BattleSide::Enemy, cell)) << "the two zones never overlap";
		}
		for (const pg::BoardCell &cell : deployment.enemyCells)
		{
			EXPECT_EQ(deployment.deploymentAxisCoordinate(cell), current.enemyRow);
		}
		// Row 0 of a side is its own outer edge, whichever edge that is.
		EXPECT_EQ(deployment.rowIndexFromOuterEdge(pg::BattleSide::Player, deployment.playerCells.front()), 0);
		EXPECT_EQ(deployment.rowIndexFromOuterEdge(pg::BattleSide::Enemy, deployment.enemyCells.front()), 0);
	}
}

TEST(DeploymentLayoutTest, ZoneCellsAreOrderedFromTheBoardCentreOutward)
{
	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5, 2));
	const pg::BoardData &board = fixture.board();

	// Depth 2, approach +Z: the player holds z = 0 and z = 1. The row nearest the fight comes first,
	// and inside a row the perpendicular coordinate rises.
	EXPECT_EQ(
		zone(board, pg::BattleSide::Player),
		(std::vector<pg::BoardCell>{
			{0, 0, 1},
			{1, 0, 1},
			{2, 0, 1},
			{3, 0, 1},
			{4, 0, 1},
			{0, 0, 0},
			{1, 0, 0},
			{2, 0, 0},
			{3, 0, 0},
			{4, 0, 0}}));

	// The enemy's own centre-first row is z = 3, and its outer edge is z = 4.
	EXPECT_EQ(
		zone(board, pg::BattleSide::Enemy),
		(std::vector<pg::BoardCell>{
			{0, 0, 3},
			{1, 0, 3},
			{2, 0, 3},
			{3, 0, 3},
			{4, 0, 3},
			{0, 0, 4},
			{1, 0, 4},
			{2, 0, 4},
			{3, 0, 4},
			{4, 0, 4}}));

	// rowIndexFromOuterEdge is step-04's authored rowsFromEnemyEdge for the enemy side: 0 is the far
	// edge, and it grows inward.
	EXPECT_EQ(board.deployment().rowIndexFromOuterEdge(pg::BattleSide::Enemy, {2, 0, 4}), 0);
	EXPECT_EQ(board.deployment().rowIndexFromOuterEdge(pg::BattleSide::Enemy, {2, 0, 3}), 1);
	EXPECT_EQ(board.deployment().perpendicularCoordinate({3, 0, 4}), 3);
}

TEST(DeploymentLayoutTest, EveryStandableLevelOfAZoneColumnIsADeploymentCell)
{
	// A bridge over the western half of the player's front row: both levels of those columns are
	// standable, so both are legal deployment cells - and the lower one is offered first.
	pgtest::BoardFixture fixture(pgtest::BoardFixture::Request{.layers = {"#####\n#####\n#####\n#####\n#####", ".....\n.....\n.....\n.....\n.....", ".....\n.....\n.....\n.....\n.....", "##...\n.....\n.....\n.....\n.....", ".....\n.....\n.....\n.....\n.....", ".....\n.....\n.....\n.....\n....."}, .deploymentDepth = 1});
	const pg::BoardData &board = fixture.board();

	EXPECT_TRUE(board.isStandable({0, 0, 0}));
	EXPECT_TRUE(board.isStandable({0, 3, 0})) << "the bridge deck";
	EXPECT_EQ(
		zone(board, pg::BattleSide::Player),
		(std::vector<pg::BoardCell>{{0, 0, 0}, {0, 3, 0}, {1, 0, 0}, {1, 3, 0}, {2, 0, 0}, {3, 0, 0}, {4, 0, 0}}))
		<< "within a row, the perpendicular coordinate leads and Y only breaks its ties";
}

TEST(DeploymentLayoutTest, ABoardThatCannotSeatBothTeamsIsATypedFailure)
{
	// Five standable cells per zone, and six creatures asked for: the board is refused rather than
	// shrunk, and nobody is dropped or stacked.
	pgtest::BoardFixture::Request request = pgtest::BoardFixture::flat(5, 5, 1, 6, 1);
	pgtest::BoardFixture tooSmall(request);
	ASSERT_FALSE(tooSmall.built());
	EXPECT_EQ(tooSmall.error().code, pg::BoardBuildErrorCode::InsufficientDeploymentCapacity);
	EXPECT_EQ(tooSmall.error().requiredPlayerSeats, 6U);
	EXPECT_EQ(tooSmall.error().availablePlayerSeats, 5U);

	// Exactly enough is enough.
	pgtest::BoardFixture exact(pgtest::BoardFixture::flat(5, 5, 1, 5, 5));
	EXPECT_TRUE(exact.built());

	// Both strips have to fit in the approach axis: 2 * depth <= extent.
	pgtest::BoardFixture tooDeep(pgtest::BoardFixture::flat(5, 5, 3));
	ASSERT_FALSE(tooDeep.built());
	EXPECT_EQ(tooDeep.error().code, pg::BoardBuildErrorCode::InvalidRequest);

	pgtest::BoardFixture noDepth(pgtest::BoardFixture::flat(5, 5, 0));
	ASSERT_FALSE(noDepth.built());
	EXPECT_EQ(noDepth.error().code, pg::BoardBuildErrorCode::InvalidRequest);

	// A vertical approach cannot even be spelled: VoxelOrientation holds the four horizontal
	// quarter-turns and nothing else, so the only place "positiveY" can be written is the JSON, where
	// the board parser rejects it against this same closed vocabulary.
	for (const pg::VoxelOrientation approach :
		 {pg::VoxelOrientation::PositiveX,
		  pg::VoxelOrientation::NegativeX,
		  pg::VoxelOrientation::PositiveZ,
		  pg::VoxelOrientation::NegativeZ})
	{
		EXPECT_TRUE(pg::isHorizontalApproach(approach));
		EXPECT_NO_THROW(auto value = pg::deploymentStrips({5, 5}, approach, 2));
		EXPECT_THROW(auto value = pg::deploymentStrips({5, 5}, approach, 3), std::invalid_argument);
	}
}

TEST(DeploymentLayoutTest, ADeploymentCellIsNotAnOccupiedCell)
{
	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5, 1, 1, 1));
	const pg::BoardData &board = fixture.board();

	// The zones are data. Nothing stands on them until a placement command succeeds.
	EXPECT_FALSE(board.occupancy().unitAt(board.deployment().playerCells.front()).has_value());
	EXPECT_EQ(board.occupancy().unitCount(), 0U);
}
