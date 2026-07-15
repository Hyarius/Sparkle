#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"

#include "board/board_data.hpp"
#include "board/board_occupancy.hpp"

#include <stdexcept>

namespace
{
	const pg::BattleUnitId First{1};
	const pg::BattleUnitId Second{2};
	const pg::BattleUnitId Ghost{9};
	const pg::BattleObjectId Trap{1};
	const pg::BattleObjectId Wall{2};
	const pg::BattleObjectId Brazier{3};

	// A 5x5 flat field with a hole at (2, *, 2).
	[[nodiscard]] pgtest::BoardFixture::Request holedField()
	{
		return pgtest::BoardFixture::Request{
			.layers =
				{"#####\n"
				 "#####\n"
				 "##.##\n"
				 "#####\n"
				 "#####",
				 ".....\n.....\n.....\n.....\n.....",
				 ".....\n.....\n.....\n.....\n....."},
			.deploymentDepth = 1};
	}
}

TEST(BoardOccupancyTest, OneUnitPerStandableCell)
{
	pgtest::BoardFixture fixture(holedField());
	const pg::BoardMutation mutation = fixture.mutation();
	const pg::BoardOccupancy &occupancy = fixture.board().occupancy();

	const pg::BoardMutationResult placed = mutation.placeUnit(First, {1, 0, 1});
	EXPECT_TRUE(placed.accepted);
	EXPECT_TRUE(placed.changed);
	EXPECT_EQ(occupancy.unitAt({1, 0, 1}), std::optional<pg::BattleUnitId>(First));
	EXPECT_EQ(occupancy.cellOf(First), std::optional<pg::BoardCell>(pg::BoardCell{1, 0, 1}));

	// Placing it exactly where it already is: accepted, and nothing changed. Not a duplicate.
	const pg::BoardMutationResult again = mutation.placeUnit(First, {1, 0, 1});
	EXPECT_TRUE(again.accepted);
	EXPECT_FALSE(again.changed);
	EXPECT_EQ(occupancy.unitCount(), 1U);

	// Somewhere else is a move, and placeUnit is not a move.
	const pg::BoardMutationResult elsewhere = mutation.placeUnit(First, {2, 0, 1});
	EXPECT_FALSE(elsewhere.accepted);
	EXPECT_EQ(elsewhere.error, std::optional(pg::BoardMutationError::UnitAlreadyPlacedElsewhere));

	const pg::BoardMutationResult onTop = mutation.placeUnit(Second, {1, 0, 1});
	EXPECT_FALSE(onTop.accepted);
	EXPECT_EQ(onTop.error, std::optional(pg::BoardMutationError::DestinationOccupied));

	const pg::BoardMutationResult inTheHole = mutation.placeUnit(Second, {2, 0, 2});
	EXPECT_FALSE(inTheHole.accepted);
	EXPECT_EQ(inTheHole.error, std::optional(pg::BoardMutationError::DestinationNotStandable));

	const pg::BoardMutationResult inTheAir = mutation.placeUnit(Second, {1, 1, 1});
	EXPECT_FALSE(inTheAir.accepted);
	EXPECT_EQ(inTheAir.error, std::optional(pg::BoardMutationError::DestinationNotStandable));

	// Not one rejection moved anything.
	EXPECT_EQ(occupancy.unitCount(), 1U);
	EXPECT_EQ(occupancy.cellOf(First), std::optional<pg::BoardCell>(pg::BoardCell{1, 0, 1}));
	EXPECT_FALSE(occupancy.cellOf(Second).has_value());
	EXPECT_TRUE(occupancy.invariantsHold());
}

TEST(BoardOccupancyTest, TheZeroIdIsTheInvalidSentinelAndIsRejected)
{
	pgtest::BoardFixture fixture(holedField());
	const pg::BoardMutation mutation = fixture.mutation();

	// A default-constructed id is zero, and zero never identifies anything.
	const pg::BoardMutationResult unit = mutation.placeUnit(pg::BattleUnitId{}, {1, 0, 1});
	EXPECT_FALSE(unit.accepted);
	EXPECT_EQ(unit.error, std::optional(pg::BoardMutationError::InvalidId));

	const pg::BoardMutationResult object = mutation.placeObject(pg::BattleObjectId{}, {1, 0, 1});
	EXPECT_FALSE(object.accepted);
	EXPECT_EQ(object.error, std::optional(pg::BoardMutationError::InvalidId));

	EXPECT_FALSE(mutation.removeUnit(pg::BattleUnitId{}));
	EXPECT_FALSE(mutation.removeObject(pg::BattleObjectId{}));
	EXPECT_EQ(fixture.board().occupancy().unitCount(), 0U);
}

TEST(BoardOccupancyTest, MoveSwapAndRemoveKeepBothMapsConsistent)
{
	pgtest::BoardFixture fixture(holedField());
	const pg::BoardMutation mutation = fixture.mutation();
	const pg::BoardOccupancy &occupancy = fixture.board().occupancy();

	ASSERT_TRUE(mutation.placeUnit(First, {1, 0, 1}).accepted);
	ASSERT_TRUE(mutation.placeUnit(Second, {3, 0, 3}).accepted);

	EXPECT_FALSE(mutation.moveUnit(Ghost, {0, 0, 0}).accepted) << "an unknown unit cannot move";
	EXPECT_EQ(mutation.moveUnit(Ghost, {0, 0, 0}).error, std::optional(pg::BoardMutationError::UnknownUnit));
	EXPECT_EQ(mutation.moveUnit(First, {2, 0, 2}).error, std::optional(pg::BoardMutationError::DestinationNotStandable));
	EXPECT_EQ(mutation.moveUnit(First, {3, 0, 3}).error, std::optional(pg::BoardMutationError::DestinationOccupied));
	EXPECT_EQ(occupancy.cellOf(First), std::optional<pg::BoardCell>(pg::BoardCell{1, 0, 1})) << "no failed move moved anything";

	const pg::BoardMutationResult stayed = mutation.moveUnit(First, {1, 0, 1});
	EXPECT_TRUE(stayed.accepted);
	EXPECT_FALSE(stayed.changed);

	ASSERT_TRUE(mutation.moveUnit(First, {0, 0, 0}).changed);
	EXPECT_EQ(occupancy.unitAt({0, 0, 0}), std::optional<pg::BattleUnitId>(First));
	EXPECT_FALSE(occupancy.unitAt({1, 0, 1}).has_value()) << "the vacated cell is free again";
	EXPECT_TRUE(occupancy.invariantsHold());

	EXPECT_EQ(mutation.swapUnits(First, First).error, std::optional(pg::BoardMutationError::CannotSwapSameUnit));
	EXPECT_EQ(mutation.swapUnits(First, Ghost).error, std::optional(pg::BoardMutationError::UnknownUnit));
	EXPECT_EQ(occupancy.cellOf(First), std::optional<pg::BoardCell>(pg::BoardCell{0, 0, 0}));

	ASSERT_TRUE(mutation.swapUnits(First, Second).changed);
	EXPECT_EQ(occupancy.cellOf(First), std::optional<pg::BoardCell>(pg::BoardCell{3, 0, 3}));
	EXPECT_EQ(occupancy.cellOf(Second), std::optional<pg::BoardCell>(pg::BoardCell{0, 0, 0}));
	EXPECT_EQ(occupancy.unitAt({3, 0, 3}), std::optional<pg::BattleUnitId>(First));
	EXPECT_EQ(occupancy.unitAt({0, 0, 0}), std::optional<pg::BattleUnitId>(Second));
	EXPECT_TRUE(occupancy.invariantsHold());

	EXPECT_TRUE(mutation.removeUnit(First));
	EXPECT_FALSE(mutation.removeUnit(First)) << "removing an unknown id is an idempotent false";
	EXPECT_FALSE(occupancy.unitAt({3, 0, 3}).has_value());
	EXPECT_EQ(occupancy.unitCount(), 1U);
	EXPECT_TRUE(occupancy.invariantsHold());
}

TEST(BoardOccupancyTest, ObjectsStackInAscendingIdOrderAndAreNotUnits)
{
	pgtest::BoardFixture fixture(holedField());
	const pg::BoardMutation mutation = fixture.mutation();
	const pg::BoardOccupancy &occupancy = fixture.board().occupancy();

	// Any number of objects on one cell, returned in ascending id order whatever order they arrived
	// in, and a unit may stand there too: the two spaces do not collide.
	ASSERT_TRUE(mutation.placeObject(Brazier, {1, 0, 1}).changed);
	ASSERT_TRUE(mutation.placeObject(Trap, {1, 0, 1}).changed);
	ASSERT_TRUE(mutation.placeObject(Wall, {1, 0, 1}).changed);
	ASSERT_TRUE(mutation.placeUnit(First, {1, 0, 1}).changed);

	const std::span<const pg::BattleObjectId> stacked = occupancy.objectsAt({1, 0, 1});
	ASSERT_EQ(stacked.size(), 3U);
	EXPECT_EQ(stacked[0], Trap);
	EXPECT_EQ(stacked[1], Wall);
	EXPECT_EQ(stacked[2], Brazier);
	EXPECT_EQ(occupancy.unitAt({1, 0, 1}), std::optional<pg::BattleUnitId>(First));
	EXPECT_EQ(occupancy.cellOf(Trap), std::optional<pg::BoardCell>(pg::BoardCell{1, 0, 1}));

	EXPECT_TRUE(occupancy.objectsAt({4, 0, 4}).empty());
	EXPECT_EQ(
		mutation.placeObject(Trap, {0, 0, 0}).error,
		std::optional(pg::BoardMutationError::ObjectAlreadyPlacedElsewhere));
	EXPECT_TRUE(mutation.placeObject(Trap, {1, 0, 1}).accepted) << "the same cell again is an idempotent no-op";
	EXPECT_FALSE(mutation.placeObject(Trap, {1, 0, 1}).changed);
	EXPECT_EQ(
		mutation.placeObject(pg::BattleObjectId{7}, {2, 0, 2}).error,
		std::optional(pg::BoardMutationError::DestinationNotStandable));

	EXPECT_TRUE(mutation.removeObject(Wall));
	EXPECT_FALSE(mutation.removeObject(Wall));
	ASSERT_EQ(occupancy.objectsAt({1, 0, 1}).size(), 2U);
	EXPECT_EQ(occupancy.objectsAt({1, 0, 1})[0], Trap);
	EXPECT_EQ(occupancy.objectsAt({1, 0, 1})[1], Brazier);
	// Removing the unit leaves its objects exactly where they were.
	EXPECT_TRUE(mutation.removeUnit(First));
	EXPECT_EQ(occupancy.objectsAt({1, 0, 1}).size(), 2U);
	EXPECT_TRUE(occupancy.invariantsHold());
}

TEST(BoardOccupancyTest, UnitAndObjectIdsCannotAliasThroughTheTypeSystem)
{
	// The tags make the two id spaces mutually unassignable, so an object id can never be mistaken
	// for the unit standing on the same cell. If this ever compiles, the ids have collapsed.
	static_assert(!std::is_convertible_v<pg::BattleObjectId, pg::BattleUnitId>);
	static_assert(!std::is_convertible_v<pg::BattleUnitId, pg::BattleObjectId>);
	static_assert(!std::is_constructible_v<pg::BattleUnitId, pg::BattleObjectId>);

	pgtest::BoardFixture fixture(holedField());
	const pg::BoardMutation mutation = fixture.mutation();
	ASSERT_TRUE(mutation.placeUnit(pg::BattleUnitId{4}, {1, 0, 1}).changed);
	ASSERT_TRUE(mutation.placeObject(pg::BattleObjectId{4}, {3, 0, 3}).changed);

	// Same numeric value, different spaces, different cells.
	EXPECT_EQ(fixture.board().occupancy().cellOf(pg::BattleUnitId{4}), std::optional<pg::BoardCell>(pg::BoardCell{1, 0, 1}));
	EXPECT_EQ(fixture.board().occupancy().cellOf(pg::BattleObjectId{4}), std::optional<pg::BoardCell>(pg::BoardCell{3, 0, 3}));
	EXPECT_TRUE(fixture.board().occupancy().objectsAt({1, 0, 1}).empty());
}
