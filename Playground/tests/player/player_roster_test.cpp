#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "core/registries.hpp"
#include "player/new_game_definition.hpp"
#include "player/player_data.hpp"
#include "support/json_fixture.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	// The real content, loaded once: the roster is about identity and order, and the one shipped
	// species is enough to exercise both.
	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries loaded = [] {
			pg::Registries result;
			result.loadAll(pg::resourceRoot() / "data");
			return result;
		}();
		return loaded;
	}

	[[nodiscard]] pg::CreatureUnit creature(std::uint64_t p_serial)
	{
		return pg::makeCreatureUnit(
			pg::CreatureInstanceId::fromSerial(p_serial),
			"training-sprout",
			std::vector<std::string>{},
			registries());
	}

	[[nodiscard]] pg::PlayerRoster filledRoster(std::size_t p_count)
	{
		pg::PlayerRoster roster;
		for (std::size_t index = 0; index < p_count; ++index)
		{
			(void)roster.add(creature(index + 1));
		}
		return roster;
	}
}

TEST(PlayerRosterTest, HoldsSixSlotsThatStartEmptyAndFillLeftToRight)
{
	pg::PlayerRoster roster;
	EXPECT_EQ(roster.team().size(), 6U);
	for (const std::optional<pg::CreatureUnit> &slot : roster.team())
	{
		EXPECT_FALSE(slot.has_value()) << "an empty slot is an ordinary nullopt, not a dummy creature";
	}

	for (std::size_t index = 0; index < 6; ++index)
	{
		const pg::PlayerRoster::Placement placement = roster.add(creature(index + 1));
		EXPECT_TRUE(placement.inTeam);
		EXPECT_EQ(placement.index, index) << "the lowest empty slot, always";
	}

	// The seventh goes to the box, in insertion order.
	const pg::PlayerRoster::Placement stored = roster.add(creature(7));
	EXPECT_FALSE(stored.inTeam);
	EXPECT_EQ(stored.index, 0U);
	EXPECT_EQ(roster.storage().size(), 1U);
	EXPECT_EQ(roster.size(), 7U);

	// A freed slot is the lowest empty one again.
	(void)roster.remove(pg::CreatureInstanceId::fromSerial(3));
	const pg::PlayerRoster::Placement refilled = roster.add(creature(8));
	EXPECT_TRUE(refilled.inTeam);
	EXPECT_EQ(refilled.index, 2U);
}

TEST(PlayerRosterTest, LooksUpByInstanceIdAndNeverBySpecies)
{
	const pg::PlayerRoster roster = filledRoster(3);

	// Three creatures, one species: identity cannot be the species, or they would be one creature.
	for (std::uint64_t serial = 1; serial <= 3; ++serial)
	{
		const pg::CreatureUnit *found = roster.find(pg::CreatureInstanceId::fromSerial(serial));
		ASSERT_NE(found, nullptr);
		EXPECT_EQ(found->id.serial(), serial);
		EXPECT_EQ(found->speciesId, "training-sprout");
	}
	EXPECT_EQ(roster.find(pg::CreatureInstanceId::fromSerial(9)), nullptr);
	EXPECT_EQ(roster.find(pg::CreatureInstanceId{}), nullptr) << "the invalid id finds nothing";

	// The same creature cannot be in the roster twice, whatever it is a copy of.
	pg::PlayerRoster duplicated = roster;
	EXPECT_THROW(auto value = duplicated.add(creature(2)), std::invalid_argument);
	EXPECT_EQ(duplicated, roster) << "a rejected add changes nothing";
}

TEST(PlayerRosterTest, MovesAndSwapsAcrossTheTeamAndTheBox)
{
	pg::PlayerRoster roster = filledRoster(8);
	ASSERT_EQ(roster.storage().size(), 2U);
	ASSERT_EQ(roster.storage()[0].id.serial(), 7U);
	ASSERT_EQ(roster.storage()[1].id.serial(), 8U);

	// Team -> box appends, keeping insertion order.
	const pg::PlayerRoster::Placement boxed = roster.moveTeamToStorage(pg::CreatureInstanceId::fromSerial(2));
	EXPECT_FALSE(boxed.inTeam);
	EXPECT_EQ(boxed.index, 2U);
	EXPECT_FALSE(roster.team()[1].has_value());

	// Box -> a named empty slot.
	const pg::PlayerRoster::Placement fielded =
		roster.moveStorageToTeam(pg::CreatureInstanceId::fromSerial(8), 1);
	EXPECT_TRUE(fielded.inTeam);
	EXPECT_EQ(fielded.index, 1U);
	ASSERT_TRUE(roster.team()[1].has_value());
	EXPECT_EQ(roster.team()[1]->id.serial(), 8U);
	// The remaining box keeps its relative order.
	ASSERT_EQ(roster.storage().size(), 2U);
	EXPECT_EQ(roster.storage()[0].id.serial(), 7U);
	EXPECT_EQ(roster.storage()[1].id.serial(), 2U);

	roster.swapTeamSlots(0, 5);
	EXPECT_EQ(roster.team()[0]->id.serial(), 6U);
	EXPECT_EQ(roster.team()[5]->id.serial(), 1U);

	// A team/box swap is in place on both sides: the box does not reshuffle.
	roster.swapTeamAndStorage(pg::CreatureInstanceId::fromSerial(6), pg::CreatureInstanceId::fromSerial(7));
	EXPECT_EQ(roster.team()[0]->id.serial(), 7U);
	EXPECT_EQ(roster.storage()[0].id.serial(), 6U);
	EXPECT_EQ(roster.storage()[1].id.serial(), 2U);

	EXPECT_EQ(roster.size(), 8U) << "no creature was created or lost by any of that";
	EXPECT_NO_THROW(roster.validate(registries()));
}

TEST(PlayerRosterTest, EveryFailedOperationLeavesADeepEqualRoster)
{
	const pg::PlayerRoster original = filledRoster(8);

	const auto expectRejected = [&original](const auto &p_operation) {
		pg::PlayerRoster roster = original;
		EXPECT_THROW(p_operation(roster), std::invalid_argument);
		EXPECT_EQ(roster, original) << "a rejected operation is a no-op, not a partial one";
	};

	// Unknown id, wrong collection, occupied target, index out of range.
	expectRejected([](pg::PlayerRoster &p_roster) {
		(void)p_roster.remove(pg::CreatureInstanceId::fromSerial(99));
	});
	expectRejected([](pg::PlayerRoster &p_roster) {
		(void)p_roster.moveTeamToStorage(pg::CreatureInstanceId::fromSerial(7));
	});
	expectRejected([](pg::PlayerRoster &p_roster) {
		(void)p_roster.moveStorageToTeam(pg::CreatureInstanceId::fromSerial(1), 0);
	});
	expectRejected([](pg::PlayerRoster &p_roster) {
		(void)p_roster.moveStorageToTeam(pg::CreatureInstanceId::fromSerial(7), 0);
	});
	expectRejected([](pg::PlayerRoster &p_roster) {
		(void)p_roster.moveStorageToTeam(pg::CreatureInstanceId::fromSerial(7), 6);
	});
	expectRejected([](pg::PlayerRoster &p_roster) {
		p_roster.swapTeamSlots(0, 6);
	});
	expectRejected([](pg::PlayerRoster &p_roster) {
		p_roster.swapTeamAndStorage(pg::CreatureInstanceId::fromSerial(7), pg::CreatureInstanceId::fromSerial(1));
	});
}

TEST(PlayerRosterTest, ValidationCatchesADerivedCacheThatTheProgressDoesNotDerive)
{
	pg::PlayerRoster roster = filledRoster(1);
	EXPECT_NO_THROW(roster.validate(registries()));

	// The cache is never authoritative: a save that claims 999 HP has to be caught, not believed.
	pg::CreatureUnit *unit = roster.findMutable(pg::CreatureInstanceId::fromSerial(1));
	ASSERT_NE(unit, nullptr);
	unit->derived.attributes.maxHealth = 999;
	EXPECT_THROW(roster.validate(registries()), std::invalid_argument);

	pg::rebuildDerivedState(*unit, registries());
	EXPECT_NO_THROW(roster.validate(registries()));

	unit->speciesId = "ghost-species";
	EXPECT_THROW(roster.validate(registries()), std::invalid_argument);
}
