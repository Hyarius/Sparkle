#include <gtest/gtest.h>

#include "abilities/ability_definition.hpp"
#include "support/json_fixture.hpp"

#include <string>
#include <string_view>
#include <variant>

namespace
{
	[[nodiscard]] pg::AbilityDefinition parseAbility(
		std::string_view p_content,
		const std::filesystem::path &p_file = "fixture.json")
	{
		const pgtest::Document document(p_content, p_file);
		pg::JsonReader reader = document.reader();
		return pg::parseAbilityDefinition(reader);
	}

	void expectRejected(std::string_view p_content, std::string_view p_because)
	{
		EXPECT_THROW((void)parseAbility(p_content), pg::JsonError) << "should be rejected: " << p_because;
	}

	constexpr std::string_view EnemyOnly = R"({
		"allowSelf": false, "allowAllies": false, "allowEnemies": true,
		"allowDefeated": false, "allowEmptyCell": false
	})";

	// The shipped Training Strike, which every case below varies one field of.
	[[nodiscard]] std::string strike(
		std::string_view p_range = R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
		std::string_view p_anchorFilter = EnemyOnly,
		std::string_view p_affectedFilter = EnemyOnly,
		std::string_view p_effects = R"([{
			"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
			"kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0
		}])",
		std::string_view p_cost = R"({"actionPoints": 2, "movementPoints": 0})")
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "ability.training-strike.name",
			"descriptionKey": "ability.training-strike.description",
			"icon": [0, 0],
			"cost": )") +
			   std::string(p_cost) + R"(, "range": )" + std::string(p_range) +
			   R"(, "area": {"shape": "single", "radius": 0}, "anchorFilter": )" + std::string(p_anchorFilter) +
			   R"(, "affectedFilter": )" + std::string(p_affectedFilter) + R"(, "effects": )" +
			   std::string(p_effects) + "}";
	}
}

TEST(AbilityDefinitionTest, ParsesTheGoldenDefinition)
{
	const pg::AbilityDefinition ability = parseAbility(strike());

	EXPECT_TRUE(ability.id.empty()) << "the registry loader assigns the filename stem";
	EXPECT_EQ(ability.displayNameKey, "ability.training-strike.name") << "a definition carries a key, not a sentence";
	EXPECT_EQ(ability.descriptionKey, "ability.training-strike.description");
	EXPECT_EQ(ability.icon, spk::Vector2Int(0, 0));
	EXPECT_EQ(ability.cost.actionPoints, 2);
	EXPECT_EQ(ability.cost.movementPoints, 0);
	EXPECT_EQ(ability.range.shape, pg::RangeShape::Diamond);
	EXPECT_EQ(ability.range.minimum, 1);
	EXPECT_EQ(ability.range.maximum, 1);
	EXPECT_TRUE(ability.range.requiresLineOfSight);
	EXPECT_EQ(ability.area.shape, pg::AreaShape::Single);
	EXPECT_EQ(ability.area.radius, 0);

	ASSERT_EQ(ability.effects.size(), 1U);
	EXPECT_EQ(ability.effects[0].id, "impact");
	EXPECT_TRUE(std::holds_alternative<pg::DamageEffectSpec>(ability.effects[0].payload));
}

TEST(AbilityDefinitionTest, KeepsTheAnchorAndAffectedFiltersDistinct)
{
	// An enemy-only anchor does not imply an enemy-only area: a bomb thrown at an enemy may
	// still hurt an ally standing next to them, and the file has to say so.
	constexpr std::string_view everyone = R"({
		"allowSelf": true, "allowAllies": true, "allowEnemies": true,
		"allowDefeated": false, "allowEmptyCell": false
	})";

	const pg::AbilityDefinition ability = parseAbility(strike(
		R"({"shape": "diamond", "minimum": 1, "maximum": 4, "requiresLineOfSight": true})",
		EnemyOnly,
		everyone));

	EXPECT_TRUE(ability.anchorFilter.allowEnemies);
	EXPECT_FALSE(ability.anchorFilter.allowAllies);
	EXPECT_TRUE(ability.affectedFilter.allowAllies) << "friendly fire is authored, not inferred";
	EXPECT_TRUE(ability.affectedFilter.allowSelf);
	EXPECT_NE(ability.anchorFilter, ability.affectedFilter);
}

TEST(AbilityDefinitionTest, PreservesTheAuthoredEffectOrderExactly)
{
	const pg::AbilityDefinition ability = parseAbility(strike(
		R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
		EnemyOnly,
		EnemyOnly,
		R"([
			{"id": "push", "type": "displace", "applyTo": "affectedUnits", "requiresLivingSource": true,
			 "direction": "awayFromSource", "distance": 1},
			{"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
			 "kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0},
			{"id": "delay", "type": "adjustTurnBar", "applyTo": "affectedUnits", "requiresLivingSource": false,
			 "deltaSeconds": -0.25}
		])"));

	ASSERT_EQ(ability.effects.size(), 3U);
	EXPECT_EQ(ability.effects[0].id, "push");
	EXPECT_EQ(ability.effects[1].id, "impact");
	EXPECT_EQ(ability.effects[2].id, "delay");
}

TEST(AbilityDefinitionTest, RejectsAnUnsupportedOrMissingVersion)
{
	pgtest::Document document(strike());
	document.value().asObject().erase("version");
	pg::JsonReader missing = document.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(missing), pg::JsonError);

	pgtest::Document future(strike());
	future.value()["version"] = 2;
	pg::JsonReader unsupported = future.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(unsupported), pg::JsonError);
}

TEST(AbilityDefinitionTest, RejectsMissingRootFieldsAndUnknownOnes)
{
	for (const std::string_view key :
		 {"displayNameKey",
		  "descriptionKey",
		  "icon",
		  "cost",
		  "range",
		  "area",
		  "anchorFilter",
		  "affectedFilter",
		  "effects"})
	{
		pgtest::Document document(strike());
		document.value().asObject().erase(std::string(key));
		pg::JsonReader reader = document.reader();
		EXPECT_THROW((void)pg::parseAbilityDefinition(reader), pg::JsonError)
			<< "should be rejected: a missing '" << key << "'";
	}

	pgtest::Document extra(strike());
	extra.value()["targetProfile"] = "enemy";
	pg::JsonReader reader = extra.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(reader), pg::JsonError)
		<< "an unknown field is an error until the schema version changes";
}

TEST(AbilityDefinitionTest, RejectsMalformedMetadata)
{
	// A text field holds a translation key, so the sentence that used to sit here is now
	// exactly what a definition may not contain.
	pgtest::Document sentence(strike());
	sentence.value()["displayNameKey"] = "Training Strike";
	pg::JsonReader sentenceReader = sentence.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(sentenceReader), pg::JsonError) << "literal text where a key belongs";

	pgtest::Document empty(strike());
	empty.value()["descriptionKey"] = "";
	pg::JsonReader emptyReader = empty.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(emptyReader), pg::JsonError) << "an empty key";

	pgtest::Document dotted(strike());
	dotted.value()["displayNameKey"] = "ability..name";
	pg::JsonReader dottedReader = dotted.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(dottedReader), pg::JsonError) << "an empty key segment";

	pgtest::Document icon(strike());
	icon.value()["icon"] = spk::Vector2Int(-1, 0);
	pg::JsonReader iconReader = icon.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(iconReader), pg::JsonError) << "a negative icon coordinate";

	pgtest::Document wideIcon(strike());
	wideIcon.value()["icon"] = spk::Vector2Int(0, 4096);
	pg::JsonReader wideReader = wideIcon.reader();
	EXPECT_THROW((void)pg::parseAbilityDefinition(wideReader), pg::JsonError) << "an icon coordinate above 4095";
}

TEST(AbilityDefinitionTest, RejectsCostsOutsideTheirBounds)
{
	expectRejected(
		strike(
			R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
			EnemyOnly,
			EnemyOnly,
			R"([{"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
			   "kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0}])",
			R"({"actionPoints": -1, "movementPoints": 0})"),
		"a negative AP cost");
	expectRejected(
		strike(
			R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
			EnemyOnly,
			EnemyOnly,
			R"([{"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
			   "kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0}])",
			R"({"actionPoints": 0, "movementPoints": 1000001})"),
		"an MP cost above the maximum");

	// A zero-cost ability is legal: a status or a passive may author one.
	EXPECT_NO_THROW((void)parseAbility(strike(
		R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
		EnemyOnly,
		EnemyOnly,
		R"([{"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0}])",
		R"({"actionPoints": 0, "movementPoints": 0})")));
}

TEST(AbilityDefinitionTest, RequiresASelfRangeToAllowTheCasterAsAnAnchor)
{
	constexpr std::string_view selfRange =
		R"({"shape": "self", "minimum": 0, "maximum": 0, "requiresLineOfSight": false})";
	constexpr std::string_view selfOnly = R"({
		"allowSelf": true, "allowAllies": false, "allowEnemies": false,
		"allowDefeated": false, "allowEmptyCell": false
	})";

	expectRejected(strike(selfRange, EnemyOnly, EnemyOnly), "a self range whose anchor filter forbids the caster");
	EXPECT_NO_THROW((void)parseAbility(strike(selfRange, selfOnly, selfOnly)));
}

TEST(AbilityDefinitionTest, RejectsAnEmptyEffectListAndDuplicateEffectIds)
{
	expectRejected(
		strike(
			R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
			EnemyOnly,
			EnemyOnly,
			"[]"),
		"an ability that does nothing");

	expectRejected(
		strike(
			R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
			EnemyOnly,
			EnemyOnly,
			R"([
				{"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
				 "kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0},
				{"id": "impact", "type": "heal", "applyTo": "sourceUnit", "requiresLivingSource": true,
				 "base": 2, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0}
			])"),
		"two effects sharing one id: an id is a stable event identity");
}

TEST(AbilityDefinitionTest, NestedErrorsKeepTheirFileAndArrayPath)
{
	try
	{
		(void)parseAbility(
			strike(
				R"({"shape": "diamond", "minimum": 1, "maximum": 1, "requiresLineOfSight": true})",
				EnemyOnly,
				EnemyOnly,
				R"([
					{"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
					 "kind": "physical", "base": 4, "strengthRatioPermille": 1000, "magicPowerRatioPermille": 0},
					{"id": "guard", "type": "applyShield", "applyTo": "sourceUnit", "requiresLivingSource": true,
					 "kind": "physical", "amount": 3, "duration": {"type": "timeline", "seconds": 0.0005}}
				])"),
			"abilities/training-strike.json");
		FAIL() << "a sub-millisecond duration must be rejected";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file(), std::filesystem::path("abilities/training-strike.json"));
		EXPECT_EQ(error.path(), "$.effects[1].duration.seconds");
	}
}
