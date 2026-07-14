#include <gtest/gtest.h>

#include "battle/battle_ids.hpp"
#include "core/deterministic_random.hpp"

#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace
{
	// The whole point of the tag parameter: an object id may never stand in for a unit id.
	static_assert(!std::is_same_v<pg::BattleUnitId, pg::BattleObjectId>);
	static_assert(!std::is_convertible_v<pg::BattleUnitId, pg::BattleObjectId>);
	static_assert(!std::is_assignable_v<pg::BattleObjectId &, pg::BattleUnitId>);
	static_assert(!std::is_constructible_v<pg::BattleId, pg::BattleUnitId>);
	// And a raw integer may not silently become an id either.
	static_assert(!std::is_convertible_v<std::uint32_t, pg::BattleUnitId>);

	static_assert(!pg::BattleId{}.valid());
	static_assert(pg::BattleId(1).valid());
	static_assert(pg::BattleId(7).value() == 7U);
	static_assert(std::is_trivially_copyable_v<pg::BattleUnitId>);
}

TEST(BattleIdsTest, DefaultConstructedIdsAreInvalid)
{
	EXPECT_FALSE(pg::BattleId{}.valid());
	EXPECT_FALSE(pg::BattleUnitId{}.valid());
	EXPECT_FALSE(pg::BattleObjectId{}.valid());
	EXPECT_EQ(pg::BattleUnitId{}.value(), 0U);
}

TEST(BattleIdsTest, ZeroIsRejectedAtTheCheckedBoundary)
{
	EXPECT_THROW((void)pg::BattleUnitId(0U), std::invalid_argument);
	EXPECT_THROW((void)pg::BattleObjectId(0U), std::invalid_argument);
	EXPECT_THROW((void)pg::BattleId(0U), std::invalid_argument);
}

TEST(BattleIdsTest, ValidIdsCompareByValue)
{
	EXPECT_EQ(pg::BattleUnitId(3), pg::BattleUnitId(3));
	EXPECT_NE(pg::BattleUnitId(3), pg::BattleUnitId(4));
	EXPECT_LT(pg::BattleUnitId(3), pg::BattleUnitId(4));
	EXPECT_GT(pg::BattleUnitId(4), pg::BattleUnitId(3));
	// The default value sorts before every valid id, so it can never win a tie break.
	EXPECT_LT(pg::BattleUnitId{}, pg::BattleUnitId(1));
}

TEST(BattleIdsTest, AllocationStartsAtOneAndIsOwnedByItsHolder)
{
	pg::BattleUnitIdAllocator units;
	EXPECT_EQ(units.allocate(), pg::BattleUnitId(1));
	EXPECT_EQ(units.allocate(), pg::BattleUnitId(2));
	EXPECT_EQ(units.allocate(), pg::BattleUnitId(3));
	EXPECT_EQ(units.allocatedCount(), 3U);

	// A second allocator is a second id space: nothing is shared through a global counter,
	// so a battle's ids never depend on how many battles ran before it.
	pg::BattleUnitIdAllocator otherUnits;
	EXPECT_EQ(otherUnits.allocate(), pg::BattleUnitId(1));

	pg::BattleObjectIdAllocator objects;
	EXPECT_EQ(objects.allocate(), pg::BattleObjectId(1));
}

TEST(BattleIdsTest, BattleIdFoldsTheCombatSeedAndRemapsTheZeroFold)
{
	// Test seam: the fold is exercised directly, so the zero case does not need a seed hunt.
	EXPECT_EQ(pg::battleIdFromMixedSeed(0U), pg::BattleId(1));
	// Halves that cancel also fold to zero and must land on the same remapped value.
	EXPECT_EQ(pg::battleIdFromMixedSeed(0x00000001'00000001ULL), pg::BattleId(1));
	EXPECT_EQ(pg::battleIdFromMixedSeed(0x00000000'0000002aULL), pg::BattleId(0x2a));
	EXPECT_EQ(pg::battleIdFromMixedSeed(0xffffffff'00000000ULL), pg::BattleId(0xffffffff));
}

TEST(BattleIdsTest, BattleIdIsAFixedFunctionOfTheCombatSeed)
{
	// Golden: deriveSeed(seed, "battle-id/v1"), folded to 32 bits. Changing either the
	// domain string or the fold rewrites every archived battle's id, so both are pinned.
	EXPECT_EQ(pg::deriveBattleId(0ULL), pg::BattleId(0x64087ceaU));
	EXPECT_EQ(pg::deriveBattleId(1234567890123456789ULL), pg::BattleId(0x7f9f85a9U));

	const std::uint64_t mixed = pg::deterministic::deriveSeed(1234567890123456789ULL, "battle-id/v1");
	EXPECT_EQ(pg::deriveBattleId(1234567890123456789ULL), pg::battleIdFromMixedSeed(mixed));
}

TEST(BattleIdsTest, IndependentDerivationsOfTheSameSeedAgree)
{
	constexpr std::uint64_t combatSeed = 0x7a11c0ffeeULL;

	const pg::BattleId first = pg::deriveBattleId(combatSeed);
	const pg::BattleId second = pg::deriveBattleId(combatSeed);
	EXPECT_EQ(first, second);
	EXPECT_TRUE(first.valid());

	EXPECT_NE(pg::deriveBattleId(combatSeed), pg::deriveBattleId(combatSeed + 1U));
}
