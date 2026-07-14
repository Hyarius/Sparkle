#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "core/registries.hpp"
#include "player/new_game_definition.hpp"
#include "player/player_data.hpp"
#include "support/json_fixture.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries loaded = [] {
			pg::Registries result;
			result.loadAll(pg::resourceRoot() / "data");
			return result;
		}();
		return loaded;
	}

	[[nodiscard]] pg::NewGameDefinition parse(std::string_view p_document)
	{
		const pgtest::Document document(p_document, "new-game.json");
		pg::JsonReader reader = document.reader();
		return pg::parseNewGameDefinition(reader);
	}

	[[nodiscard]] std::string starters(std::string_view p_entries)
	{
		return std::string(R"({"version": 1, "starterTeam": [)") + std::string(p_entries) + "]}";
	}

	constexpr std::string_view Sprout = R"({"species": "training-sprout", "completedFeatNodes": []})";
}

TEST(NewGameTest, DealsSerialsAndTeamSlotsInAuthoredOrder)
{
	const pg::NewGameDefinition definition =
		parse(starters(std::string(Sprout) + "," + std::string(Sprout) + "," + std::string(Sprout)));
	const pg::PlayerData player = pg::makeNewPlayerData(definition, registries());

	// Serials 1..N, in array order, and the counter parked on N+1.
	for (std::size_t slot = 0; slot < 3; ++slot)
	{
		ASSERT_TRUE(player.roster.team()[slot].has_value());
		EXPECT_EQ(player.roster.team()[slot]->id.serial(), slot + 1);
		EXPECT_EQ(player.roster.team()[slot]->speciesId, "training-sprout");
		EXPECT_EQ(player.roster.team()[slot]->derived.formId, "base");
	}
	EXPECT_EQ(player.roster.team()[0]->id.string(), "creature-0000000000000001");
	EXPECT_FALSE(player.roster.team()[3].has_value());
	EXPECT_EQ(player.nextCreatureSerial, 4U);

	// A new game owns nothing else yet.
	EXPECT_TRUE(player.roster.storage().empty());
	EXPECT_EQ(player.encounterSerial, 0U);
	EXPECT_TRUE(player.clearedTrainerIds.empty());
	EXPECT_TRUE(player.clearedGymIds.empty());
	EXPECT_TRUE(player.clearedSpecialEncounterIds.empty());
	EXPECT_EQ(player.encounterTier(), 0U);
	EXPECT_NO_THROW(player.roster.validate(registries()));
}

TEST(NewGameTest, RejectsTheSchemaFailuresContextually)
{
	EXPECT_THROW(auto value = parse(starters("")), pg::JsonError) << "a run starts with at least one creature";
	EXPECT_THROW(
		auto value = parse(starters(
			std::string(Sprout) + "," + std::string(Sprout) + "," + std::string(Sprout) + "," + std::string(Sprout) +
			"," + std::string(Sprout) + "," + std::string(Sprout) + "," + std::string(Sprout))),
		pg::JsonError)
		<< "seven starters do not fit six slots";
	EXPECT_THROW(auto value = parse(R"({"version": 2, "starterTeam": []})"), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(R"({"version": 1, "starterTeam": [{"species": "training-sprout",
			"completedFeatNodes": [], "id": "creature-0000000000000001"}]})"),
		pg::JsonError)
		<< "identity is the run's to allocate, not the content's to author";
	EXPECT_THROW(
		auto value = parse(starters(R"({"species": "training-sprout", "completedFeatNodes": ["root", "root"]})")),
		pg::JsonError);

	// Unknown species and illegal presets fail against the registries, naming the entry.
	try
	{
		const pg::PlayerData player =
			pg::makeNewPlayerData(parse(starters(R"({"species": "ghost", "completedFeatNodes": []})")), registries());
		FAIL() << "an unknown starter species must fail the boot";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(error.path().find("starterTeam[0]"), std::string::npos);
		EXPECT_NE(error.message().find("ghost"), std::string::npos);
	}

	EXPECT_THROW(
		auto value = pg::makeNewPlayerData(
			parse(starters(R"({"species": "training-sprout", "completedFeatNodes": ["learn-snare"]})")),
			registries()),
		pg::JsonError)
		<< "learn-snare gates on a completed node, so it cannot be the only one completed";
}

TEST(NewGameTest, AFailedRecruitReservesNoSerialAndChangesNothing)
{
	pg::PlayerData player = pg::makeNewPlayerData(parse(starters(Sprout)), registries());
	const pg::PlayerData before = player;

	EXPECT_THROW(
		auto value = pg::createAndAddCreature(player, "ghost", std::vector<std::string>{}, registries()),
		std::invalid_argument);
	EXPECT_EQ(player, before) << "the working copy was discarded, and with it the serial it had reserved";
	EXPECT_EQ(player.nextCreatureSerial, 2U);

	// A legal recruit does advance it, once.
	const pg::PlayerRoster::Placement placement =
		pg::createAndAddCreature(player, "training-sprout", std::vector<std::string>{}, registries());
	EXPECT_TRUE(placement.inTeam);
	EXPECT_EQ(placement.index, 1U);
	EXPECT_EQ(player.nextCreatureSerial, 3U);
	EXPECT_NE(player, before);

	// And the original value is untouched by any of it: PlayerData is a value, not a service.
	EXPECT_EQ(before.nextCreatureSerial, 2U);
	EXPECT_EQ(before.roster.size(), 1U);
}

TEST(NewGameTest, AllocationRefusesToReuseOrOverflowASerial)
{
	pg::PlayerData player;

	EXPECT_EQ(player.allocateCreatureId().serial(), 1U);
	EXPECT_EQ(player.nextCreatureSerial, 2U);

	// A counter that fell behind the roster must not hand out an id twice.
	player.roster.add(pg::makeCreatureUnit(
		pg::CreatureInstanceId::fromSerial(2),
		"training-sprout",
		std::vector<std::string>{},
		registries()));
	EXPECT_THROW(auto value = player.allocateCreatureId(), std::invalid_argument);
	EXPECT_EQ(player.nextCreatureSerial, 2U) << "a rejected allocation advances nothing";

	// The end of the range is an error, not a wrap back onto the invalid serial zero.
	pg::PlayerData exhausted;
	exhausted.nextCreatureSerial = std::numeric_limits<std::uint64_t>::max();
	EXPECT_EQ(exhausted.allocateCreatureId().serial(), std::numeric_limits<std::uint64_t>::max());
	EXPECT_EQ(exhausted.nextCreatureSerial, 0U);
	EXPECT_THROW(auto value = exhausted.allocateCreatureId(), std::overflow_error);
}

TEST(NewGameTest, TheEncounterTierIsTheClearedGymCountAndCannotDrift)
{
	pg::PlayerData player;
	EXPECT_EQ(player.encounterTier(), 0U);

	for (int gym = 1; gym <= 8; ++gym)
	{
		player.clearedGymIds.insert("gym-" + std::to_string(gym));
		EXPECT_EQ(player.encounterTier(), static_cast<std::uint32_t>(gym));
	}

	// Tier 9 is reserved for a postgame state that does not exist, so the count clamps at 8.
	player.clearedGymIds.insert("gym-9");
	EXPECT_EQ(player.encounterTier(), 8U);

	// Trainers and specials are cleared separately and never move the tier.
	player.clearedTrainerIds.insert("rival");
	player.clearedSpecialEncounterIds.insert("legend");
	EXPECT_EQ(player.encounterTier(), 8U);
}

TEST(NewGameTest, TheShippedNewGameConfigIsTheOneMainBootsFrom)
{
	const pg::PlayerData player = pg::makeNewPlayerData(registries().newGame(), registries());

	ASSERT_EQ(registries().newGame().starterTeam.size(), 1U);
	EXPECT_EQ(registries().newGame().starterTeam[0].speciesId, "training-sprout");
	EXPECT_TRUE(registries().newGame().starterTeam[0].completedFeatNodeIds.empty());

	ASSERT_TRUE(player.roster.team()[0].has_value());
	EXPECT_EQ(player.roster.team()[0]->id.string(), "creature-0000000000000001");
	EXPECT_EQ(player.roster.team()[0]->derived.abilityIds, (std::vector<std::string>{"training-strike"}));
	EXPECT_EQ(player.nextCreatureSerial, 2U);
	// The world cells stay at the origin until the scene commits a spawn.
	EXPECT_EQ(player.playerCell, spk::Vector3Int{});
	EXPECT_EQ(player.lastHealPoint, spk::Vector3Int{});
}
