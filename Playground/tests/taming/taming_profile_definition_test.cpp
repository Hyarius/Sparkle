#include <gtest/gtest.h>

#include "support/json_fixture.hpp"
#include "taming/taming_profile_definition.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace
{
	// Taming profiles are embedded, never files of their own, so the fixture wraps one in a
	// species-shaped document: absent means untameable.
	[[nodiscard]] std::string species(std::string_view p_profile)
	{
		if (p_profile.empty())
		{
			return R"({"id": "training-creature"})";
		}
		return std::string(R"({"id": "training-creature", "tamingProfile": )") + std::string(p_profile) + "}";
	}

	[[nodiscard]] std::optional<pg::TamingProfileDefinition> parse(std::string_view p_species)
	{
		const pgtest::Document document(p_species, "training-creature.json");
		const pg::JsonReader reader = document.reader();
		if (!reader.contains("tamingProfile"))
		{
			return std::nullopt;
		}

		pg::JsonReader profileReader = reader.child("tamingProfile");
		return pg::parseTamingProfile(profileReader, pg::ConditionLimits{});
	}

	// The GDD's first taming example, and the reason condition JSON never names an absolute side:
	// here the subject is the wild creature, so the player is its "opponentTeam".
	constexpr std::string_view ImpressWithMagic = R"({
		"conditions": [
			{
				"id": "impress-with-magic",
				"type": "damage",
				"descriptionKey": "taming.training-creature.impress.description",
				"window": "ability",
				"windowActor": "opponentTeam",
				"requiredWindowCount": 1,
				"windowMode": "cumulative",
				"actor": "opponentTeam",
				"role": "source",
				"counterpart": "subject",
				"kind": "magical",
				"component": "health",
				"targetHadShield": "any",
				"sourceAbilities": [],
				"aggregation": "maximum",
				"comparison": "greaterThan",
				"threshold": 8
			}
		]
	})";
}

TEST(TamingProfileDefinitionTest, AnAbsentProfileMeansUntameable)
{
	EXPECT_FALSE(parse(species("")).has_value());

	// An explicit null is not "absent": in schema version 1 it is a value of the wrong type.
	EXPECT_THROW(auto value = parse(species("null")), pg::JsonError);
}

TEST(TamingProfileDefinitionTest, ParsesTheSharedConditionTreeWithRelativeSides)
{
	const std::optional<pg::TamingProfileDefinition> profile = parse(species(ImpressWithMagic));
	ASSERT_TRUE(profile.has_value());
	ASSERT_EQ(profile->conditions.size(), 1U);

	const pg::ConditionSpec &condition = profile->conditions[0];
	EXPECT_EQ(condition.id, "impress-with-magic");

	// Exactly the Feat parser, exactly the same closed variant.
	const auto &damage = std::get<pg::DamageConditionSpec>(condition.payload);
	EXPECT_EQ(damage.actor, pg::ConditionUnitSet::OpponentTeam) << "the player's team, seen from the wild creature";
	EXPECT_EQ(damage.counterpart, pg::ConditionUnitSet::Subject);
	EXPECT_EQ(damage.leaf.window, pg::ConditionWindow::Ability);
	EXPECT_EQ(damage.leaf.windowActor, pg::ConditionUnitSet::OpponentTeam);
	EXPECT_EQ(damage.metric.aggregation, pg::ConditionAggregation::Maximum);
	EXPECT_EQ(damage.metric.threshold, 8);
}

TEST(TamingProfileDefinitionTest, AcceptsEveryWindowIncludingTheEncounterLocalGameWindow)
{
	const auto profileWith = [](std::string_view p_window, std::string_view p_actor, std::string_view p_count) {
		return species(
			std::string(R"({"conditions": [{"id": "approach", "type": "position",
			"descriptionKey": "taming.fixture.description", "window": ")") +
			std::string(p_window) + R"(", "windowActor": ")" + std::string(p_actor) +
			R"(", "requiredWindowCount": )" + std::string(p_count) +
			R"(, "windowMode": "cumulative", "sample": "turnStart", "actor": "opponentTeam",
			   "relativeTo": "subject", "comparison": "atMost", "distance": 1}]})");
	};

	EXPECT_TRUE(parse(profileWith("ability", "opponentTeam", "1")).has_value());
	EXPECT_TRUE(parse(profileWith("turn", "opponentTeam", "3")).has_value());
	EXPECT_TRUE(parse(profileWith("fight", "any", "1")).has_value());
	// Accepted, but battle-local: a taming "game" window is reset at encounter construction, so
	// it means the current encounter. Partial taming progress is never saved.
	EXPECT_TRUE(parse(profileWith("game", "any", "1")).has_value());
}

TEST(TamingProfileDefinitionTest, RejectsAnEmptyProfileAndTheUsualConditionFailures)
{
	// A present profile means tameable, so an empty list is an authoring error, not "always".
	EXPECT_THROW(auto value = parse(species(R"({"conditions": []})")), pg::JsonError);
	EXPECT_THROW(auto value = parse(species(R"({})")), pg::JsonError);
	EXPECT_THROW(auto value = parse(species(R"({"conditions": [], "chance": 0.25})")), pg::JsonError)
		<< "there is no probability, item or capture action in this model";

	// The shared parser brings its own strictness: duplicate ids through the tree, unknown types.
	const std::string duplicated = species(R"({"conditions": [
		{"id": "twice", "type": "battleOutcome", "descriptionKey": "taming.fixture.description",
		 "window": "fight", "windowActor": "any", "requiredWindowCount": 1, "windowMode": "cumulative",
		 "outcome": "completed", "requireSubjectActive": false},
		{"id": "twice", "type": "battleOutcome", "descriptionKey": "taming.fixture.description",
		 "window": "fight", "windowActor": "any", "requiredWindowCount": 1, "windowMode": "cumulative",
		 "outcome": "draw", "requireSubjectActive": false}]})");
	EXPECT_THROW(auto value = parse(duplicated), pg::JsonError);
}
