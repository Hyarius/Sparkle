#include <gtest/gtest.h>

#include "board/handcrafted_battle_board_definition.hpp"
#include "encounters/encounter_definition.hpp"
#include "support/json_fixture.hpp"

#include "structures/voxel/spk_prefab.hpp"

#include <array>
#include <string>
#include <string_view>
#include <variant>

namespace
{
	// One handcrafted arena to reference: 13x13, the player approaching along +Z, so the enemy
	// deploys on z = 11 and z = 12.
	[[nodiscard]] const pg::Registry<pg::HandcraftedBattleBoardDefinition> &boards()
	{
		static const pg::Registry<pg::HandcraftedBattleBoardDefinition> registry = [] {
			pg::HandcraftedBattleBoardDefinition arena;
			arena.id = "verdant-trial-arena";
			arena.displayNameKey = "battle-board.verdant-trial-arena.name";
			arena.size = spk::Vector3Int(13, 3, 13);
			arena.geometryPrefabId = "verdant-trial-arena-geometry";
			arena.playerApproach = spk::VoxelOrientation::PositiveZ;
			arena.deploymentDepth = 2;

			pg::Registry<pg::HandcraftedBattleBoardDefinition> result;
			result.add("verdant-trial-arena", std::move(arena));
			return result;
		}();
		return registry;
	}

	constexpr std::string_view ByLine = R"({"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"})";

	[[nodiscard]] std::string liveWorld(std::string_view p_placement = ByLine, std::string_view p_size = "[11, 11]")
	{
		return std::string(R"({"type": "liveWorld", "size": )") + std::string(p_size) +
			   R"(, "deploymentDepth": 2, "opponentPlacement": )" + std::string(p_placement) + "}";
	}

	[[nodiscard]] std::string handcrafted(std::string_view p_placement = ByLine)
	{
		return std::string(R"({"type": "handcrafted", "definition": "verdant-trial-arena",
			"opponentPlacement": )") +
			   std::string(p_placement) + "}";
	}

	[[nodiscard]] std::string member(std::string_view p_id)
	{
		return std::string(R"({"id": ")") + std::string(p_id) +
			   R"(", "species": "training-sprout", "ai": "training-aggressive", "completedFeatNodes": []})";
	}

	[[nodiscard]] std::string team(std::string_view p_id, std::string_view p_weight, std::string_view p_members)
	{
		return std::string(R"({"id": ")") + std::string(p_id) +
			   R"(", "displayNameKey": "encounter.fixture.team.name", "weight": )" + std::string(p_weight) +
			   R"(, "members": [)" + std::string(p_members) + "]}";
	}

	[[nodiscard]] std::string tiers(std::string_view p_teams, std::string_view p_tier = "0")
	{
		return std::string(R"([{"tier": )") + std::string(p_tier) + R"(, "teams": [)" + std::string(p_teams) + "]}]";
	}

	[[nodiscard]] std::string encounter(
		std::string_view p_kind = "wild",
		std::string_view p_taming = "true",
		std::string_view p_repeatable = "true",
		std::string_view p_board = "",
		std::string_view p_tiers = "",
		std::string_view p_chance = "1000")
	{
		const std::string board = p_board.empty() ? liveWorld() : std::string(p_board);
		const std::string tierArray =
			p_tiers.empty() ? tiers(team("lone-sprout", "1", member("sprout-a"))) : std::string(p_tiers);

		return std::string(R"({"version": 1, "displayNameKey": "encounter.fixture.name", "kind": ")") +
			   std::string(p_kind) + R"(", "allowsTaming": )" + std::string(p_taming) + R"(, "repeatable": )" +
			   std::string(p_repeatable) + R"(, "triggerChancePermille": )" + std::string(p_chance) +
			   R"(, "board": )" + board + R"(, "tiers": )" + tierArray + "}";
	}

	[[nodiscard]] pg::EncounterDefinition parse(const std::string &p_document)
	{
		const pgtest::Document document(p_document, "training-wild.json");
		pg::JsonReader reader = document.reader();
		pg::EncounterDefinition definition = pg::parseEncounterDefinition(reader, boards());
		definition.id = "training-wild";
		return definition;
	}
}

TEST(EncounterDefinitionTest, ParsesTheWildAlternativeWithItsBoardAndRoster)
{
	const pg::EncounterDefinition parsed = parse(encounter());

	EXPECT_EQ(parsed.kind, pg::EncounterKind::Wild);
	EXPECT_TRUE(parsed.allowsTaming);
	EXPECT_TRUE(parsed.repeatable);
	EXPECT_EQ(parsed.triggerChancePermille, 1000U);

	const auto &live = std::get<pg::LiveWorldBoardPolicyDefinition>(parsed.board);
	EXPECT_EQ(live.size, (std::array<int, 2>{11, 11}));
	EXPECT_EQ(live.deploymentDepth, 2);
	const auto &byLine = std::get<pg::ByLineOpponentPlacementPolicy>(live.opponentPlacement);
	EXPECT_EQ(byLine.rowsFromEnemyEdge, 0);
	EXPECT_EQ(byLine.order, pg::OpponentLineOrder::CenterOut);

	ASSERT_EQ(parsed.tiers.size(), 1U);
	ASSERT_EQ(parsed.tiers[0].teams.size(), 1U);
	ASSERT_EQ(parsed.tiers[0].teams[0].members.size(), 1U);
	// Member order is the enemy roster order, and therefore the selector tie-break order.
	EXPECT_EQ(parsed.tiers[0].teams[0].members[0].id, "sprout-a");
	EXPECT_EQ(parsed.tiers[0].teams[0].members[0].speciesId, "training-sprout");
	EXPECT_EQ(parsed.tiers[0].teams[0].members[0].aiBehaviourId, "training-aggressive");
	EXPECT_EQ(pg::effectiveBoardSize(parsed.board, boards()), (std::array<int, 2>{11, 11}));
	EXPECT_EQ(pg::effectiveDeploymentDepth(parsed.board, boards()), 2);
	EXPECT_EQ(pg::largestTeamSize(parsed), 1U);
}

TEST(EncounterDefinitionTest, LocksEveryKindToItsBoardTamingAndRepeatability)
{
	// Wild and trainer overlay the live world; gym and special fight in an arena; debug may do
	// either, without leaking the other alternative's fields.
	EXPECT_NO_THROW(auto value = parse(encounter("wild", "true", "true")));
	EXPECT_NO_THROW(auto value = parse(encounter("wild", "false", "true")));
	EXPECT_NO_THROW(auto value = parse(encounter("trainer", "false", "false")));
	EXPECT_NO_THROW(auto value = parse(encounter("gym", "false", "false", handcrafted())));
	EXPECT_NO_THROW(auto value = parse(encounter("special", "false", "false", handcrafted())));
	EXPECT_NO_THROW(auto value = parse(encounter("debug", "false", "true")));
	EXPECT_NO_THROW(auto value = parse(encounter("debug", "false", "true", handcrafted())));

	EXPECT_THROW(auto value = parse(encounter("gym", "false", "false")), pg::JsonError)
		<< "a gym fights in a handcrafted arena";
	EXPECT_THROW(auto value = parse(encounter("special", "false", "false")), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", handcrafted())), pg::JsonError)
		<< "a wild encounter overlays the world the player walked into";
	EXPECT_THROW(auto value = parse(encounter("trainer", "false", "false", handcrafted())), pg::JsonError);

	// Only a wild encounter may be tamed.
	EXPECT_THROW(auto value = parse(encounter("trainer", "true", "false")), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("gym", "true", "false", handcrafted())), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("debug", "true", "true")), pg::JsonError);

	// Repeatability is kind-locked even though it stays explicit in the file.
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "false")), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("trainer", "false", "true")), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("gym", "false", "true", handcrafted())), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("debug", "false", "false")), pg::JsonError);
}

TEST(EncounterDefinitionTest, KeepsTheTwoBoardAlternativesStrictlyApart)
{
	// A handcrafted policy may not repeat the size, depth or approach it inherits.
	EXPECT_THROW(
		auto value = parse(encounter(
			"gym",
			"false",
			"false",
			R"({"type": "handcrafted", "definition": "verdant-trial-arena", "size": [13, 13],
			   "opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter(
			"gym",
			"false",
			"false",
			R"({"type": "handcrafted", "definition": "verdant-trial-arena", "playerApproach": "positiveZ",
			   "opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}})")),
		pg::JsonError);
	// And a live-world one carries no definition.
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			R"({"type": "liveWorld", "definition": "verdant-trial-arena", "size": [11, 11], "deploymentDepth": 2,
			   "opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}})")),
		pg::JsonError);

	EXPECT_THROW(
		auto value = parse(encounter(
			"gym",
			"false",
			"false",
			R"({"type": "handcrafted", "definition": "ghost-arena",
			   "opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}})")),
		pg::JsonError);

	// Live-world board bounds are the arena bounds: odd sides in [5, 31], both strips fitting.
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", liveWorld(ByLine, "[10, 11]"))), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", liveWorld(ByLine, "[3, 11]"))), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", liveWorld(ByLine, "[33, 11]"))), pg::JsonError);
	// A depth of 2 does not fit twice into a 3-wide axis, whichever axis the approach lands on.
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			R"({"type": "liveWorld", "size": [5, 11], "deploymentDepth": 3,
			   "opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}})")),
		pg::JsonError);
}

TEST(EncounterDefinitionTest, EnforcesTierWeightTeamAndMemberInvariants)
{
	// Tiers: non-empty, first 0, strictly increasing, at most 9.
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", "", "[]")), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter("wild", "true", "true", "", tiers(team("t", "1", member("m")), "1"))),
		pg::JsonError)
		<< "a sparse table with no tier 0 has nothing to fall back to";
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			"",
			std::string(R"([{"tier": 0, "teams": [)") + team("a", "1", member("m")) +
				R"(]}, {"tier": 0, "teams": [)" + team("b", "1", member("m")) + "]}]")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			"",
			std::string(R"([{"tier": 0, "teams": [)") + team("a", "1", member("m")) +
				R"(]}, {"tier": 10, "teams": [)" + team("b", "1", member("m")) + "]}]")),
		pg::JsonError);

	// Weights are positive integers, and the tier's sum is checked in 64 bits.
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", "", tiers(team("t", "0", member("m"))))), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter("wild", "true", "true", "", tiers(team("t", "1000000001", member("m"))))),
		pg::JsonError);
	EXPECT_NO_THROW(
		auto value = parse(encounter("wild", "true", "true", "", tiers(team("t", "1000000000", member("m"))))));

	// Teams: at least one per tier, unique ids, 1 to 6 members with unique local ids.
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", "", R"([{"tier": 0, "teams": []}])")), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			"",
			tiers(team("same", "1", member("a")) + "," + team("same", "1", member("b"))))),
		pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", "", tiers(team("t", "1", "")))), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			"",
			tiers(team("t", "1", member("a") + "," + member("a"))))),
		pg::JsonError);

	// The chance is a permille, and only a Bush ever rolls it.
	EXPECT_NO_THROW(auto value = parse(encounter("wild", "true", "true", "", "", "0")));
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", "", "", "1001")), pg::JsonError);
	EXPECT_THROW(auto value = parse(encounter("wild", "true", "true", "", "", "-1")), pg::JsonError);
}

TEST(EncounterDefinitionTest, ValidatesEveryPlacementAlternativeAgainstTheEffectiveBoard)
{
	const std::string sixMembers =
		member("a") + "," + member("b") + "," + member("c") + "," + member("d") + "," + member("e") + "," + member("f");

	// By line: the row offset lives inside the enemy zone, and the rows it can still reach have to
	// hold the largest team.
	EXPECT_NO_THROW(auto value = parse(encounter("wild", "true", "true", liveWorld(R"({"type": "byLine", "rowsFromEnemyEdge": 1, "order": "leftToRight"})"))));
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			liveWorld(R"({"type": "byLine", "rowsFromEnemyEdge": 2, "order": "centerOut"})"))),
		pg::JsonError)
		<< "the zone is two rows deep, so the offsets are 0 and 1";
	// Enumeration walks inward and never back toward the edge, so a team of six does not fit in the
	// one 5-wide row that an offset of 1 leaves reachable.
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			liveWorld(R"({"type": "byLine", "rowsFromEnemyEdge": 1, "order": "centerOut"})", "[5, 5]"),
			tiers(team("six", "1", sixMembers)))),
		pg::JsonError);

	// Fixed: unique, inside the extent, and enough of them for the largest team.
	EXPECT_NO_THROW(auto value = parse(encounter("wild", "true", "true", liveWorld(R"({"type": "fixed", "cells": [[5, 10], [4, 10], [6, 10]]})"))));
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			liveWorld(R"({"type": "fixed", "cells": [[5, 10], [5, 10]]})"))),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			liveWorld(R"({"type": "fixed", "cells": [[11, 10]]})"))),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			liveWorld(R"({"type": "fixed", "cells": [[5, 10], [4, 10]]})"),
			tiers(team("six", "1", sixMembers)))),
		pg::JsonError);

	// A handcrafted board knows its approach, so its fixed cells have to be in the enemy strip
	// (z = 11 or z = 12) rather than merely on the board.
	EXPECT_NO_THROW(auto value = parse(encounter("gym", "false", "false", handcrafted(R"({"type": "fixed", "cells": [[6, 12], [5, 11]]})"))));
	EXPECT_THROW(
		auto value = parse(encounter(
			"gym",
			"false",
			"false",
			handcrafted(R"({"type": "fixed", "cells": [[6, 12], [6, 5]]})"))),
		pg::JsonError)
		<< "z = 5 is the middle of the arena, not the enemy's edge";

	// Seeded random takes no field at all, and draws from the whole enemy zone.
	EXPECT_NO_THROW(auto value = parse(encounter("wild", "true", "true", liveWorld(R"({"type": "seededRandom"})"))));
	EXPECT_THROW(
		auto value = parse(encounter(
			"wild",
			"true",
			"true",
			liveWorld(R"({"type": "seededRandom", "seed": 42})"))),
		pg::JsonError)
		<< "the battle RNG supplies the seed; content never does";
	EXPECT_THROW(
		auto value = parse(encounter("wild", "true", "true", liveWorld(R"({"type": "randomWalk"})"))),
		pg::JsonError);
}
