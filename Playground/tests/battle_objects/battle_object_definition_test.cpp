#include <gtest/gtest.h>

#include "battle_objects/battle_object_definition.hpp"
#include "support/json_fixture.hpp"

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace
{
	[[nodiscard]] pg::BattleObjectDefinition parseObject(std::string_view p_content)
	{
		const pgtest::Document document(p_content);
		pg::JsonReader reader = document.reader();
		return pg::parseBattleObjectDefinition(reader);
	}

	void expectRejected(std::string_view p_content, std::string_view p_because)
	{
		EXPECT_THROW((void)parseObject(p_content), pg::JsonError) << "should be rejected: " << p_because;
	}

	constexpr std::string_view EnemyOnly = R"({
		"allowSelf": false, "allowAllies": false, "allowEnemies": true,
		"allowDefeated": false, "allowEmptyCell": false
	})";

	constexpr std::string_view SnareTrigger = R"([{
		"id": "on-enter",
		"when": "unitEnteredCell",
		"targetFilter": {
			"allowSelf": false, "allowAllies": false, "allowEnemies": true,
			"allowDefeated": false, "allowEmptyCell": false
		},
		"maxTriggers": 1,
		"removeWhenExhausted": true,
		"effects": [{
			"id": "movement-penalty", "type": "applyNextActivationPenalty", "applyTo": "primaryUnit",
			"requiresLivingSource": false, "resource": "movementPoints", "amount": 1
		}]
	}])";

	// The shipped Training Snare, which every case below varies one block of.
	[[nodiscard]] std::string snare(
		std::string_view p_triggers = SnareTrigger,
		std::string_view p_blocksMovement = "false",
		std::string_view p_blocksLineOfSight = "false",
		std::string_view p_tags = R"(["trap", "snare"])")
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "battle-object.training-snare-object.name",
			"descriptionKey": "battle-object.training-snare-object.description",
			"icon": [2, 0],
			"tags": )") +
			   std::string(p_tags) + R"(, "blocksMovement": )" + std::string(p_blocksMovement) +
			   R"(, "blocksLineOfSight": )" + std::string(p_blocksLineOfSight) + R"(, "triggers": )" +
			   std::string(p_triggers) + "}";
	}
}

TEST(BattleObjectDefinitionTest, ParsesTheGoldenDefinition)
{
	const pg::BattleObjectDefinition object = parseObject(snare());

	EXPECT_EQ(object.displayNameKey, "battle-object.training-snare-object.name")
		<< "a definition carries a key, not a sentence";
	EXPECT_EQ(object.icon, spk::Vector2Int(2, 0));
	EXPECT_EQ(object.tags, (std::vector<std::string>{"trap", "snare"}));
	EXPECT_FALSE(object.blocksMovement);
	EXPECT_FALSE(object.blocksLineOfSight);

	ASSERT_EQ(object.triggers.size(), 1U);
	const pg::BattleObjectTriggerSpec &trigger = object.triggers[0];
	EXPECT_EQ(trigger.id, "on-enter");
	EXPECT_EQ(trigger.trigger, pg::BattleObjectTrigger::UnitEnteredCell);
	EXPECT_TRUE(trigger.targetFilter.allowEnemies);
	EXPECT_FALSE(trigger.targetFilter.allowAllies);
	EXPECT_EQ(trigger.maxTriggers, 1U);
	EXPECT_TRUE(trigger.removeWhenExhausted);

	ASSERT_EQ(trigger.effects.size(), 1U);
	EXPECT_EQ(trigger.effects[0].id, "movement-penalty");
	EXPECT_EQ(trigger.effects[0].applyTo, pg::EffectApplyTo::PrimaryUnit);
	EXPECT_FALSE(trigger.effects[0].requiresLivingSource) << "a snare outlives the caster that placed it";

	const pg::ApplyNextActivationPenaltyEffectSpec &penalty =
		std::get<pg::ApplyNextActivationPenaltyEffectSpec>(trigger.effects[0].payload);
	EXPECT_EQ(penalty.resource, pg::BattleResource::MovementPoints);
	EXPECT_EQ(penalty.amount, 1);
}

TEST(BattleObjectDefinitionTest, ReadsAllFiveTriggerMoments)
{
	const auto triggered = [](std::string_view p_when) {
		return std::string(R"([{"id": "react", "when": ")") + std::string(p_when) + R"(", "targetFilter": )" +
			   std::string(EnemyOnly) +
			   R"(, "maxTriggers": 0, "removeWhenExhausted": false, "effects": [{"id": "zap", "type": "damage",
			   "applyTo": "primaryUnit", "requiresLivingSource": false, "kind": "magical", "base": 2,
			   "strengthRatioPermille": 0, "magicPowerRatioPermille": 0}]}])";
	};

	const std::array<std::pair<std::string_view, pg::BattleObjectTrigger>, 5> moments{
		std::pair{std::string_view("unitEnteredCell"), pg::BattleObjectTrigger::UnitEnteredCell},
		std::pair{std::string_view("unitLeftCell"), pg::BattleObjectTrigger::UnitLeftCell},
		std::pair{std::string_view("unitEndedMoveOnCell"), pg::BattleObjectTrigger::UnitEndedMoveOnCell},
		std::pair{std::string_view("unitActivationStartedOnCell"), pg::BattleObjectTrigger::UnitActivationStartedOnCell},
		std::pair{std::string_view("unitActivationEndedOnCell"), pg::BattleObjectTrigger::UnitActivationEndedOnCell}};

	for (const auto &[literal, trigger] : moments)
	{
		const pg::BattleObjectDefinition object = parseObject(snare(triggered(literal)));
		ASSERT_EQ(object.triggers.size(), 1U) << literal;
		EXPECT_EQ(object.triggers[0].trigger, trigger) << literal;
		EXPECT_EQ(object.triggers[0].maxTriggers, 0U) << "zero means unlimited for the instance lifetime";
	}

	expectRejected(snare(triggered("unitDefeatedOnCell")), "a trigger moment that does not exist");
}

TEST(BattleObjectDefinitionTest, ReadsTheBlockingFlags)
{
	const pg::BattleObjectDefinition wall = parseObject(snare(SnareTrigger, "true", "true", R"(["wall"])"));
	EXPECT_TRUE(wall.blocksMovement);
	EXPECT_TRUE(wall.blocksLineOfSight);

	const pg::BattleObjectDefinition glass = parseObject(snare(SnareTrigger, "true", "false", R"(["wall"])"));
	EXPECT_TRUE(glass.blocksMovement);
	EXPECT_FALSE(glass.blocksLineOfSight) << "a blocking object need not block sight";
}

TEST(BattleObjectDefinitionTest, RejectsAnUnlimitedTriggerThatAlsoRemovesOnExhaustion)
{
	const auto limited = [](std::string_view p_maxTriggers, std::string_view p_remove) {
		return std::string(R"([{"id": "react", "when": "unitEnteredCell", "targetFilter": )") +
			   std::string(EnemyOnly) + R"(, "maxTriggers": )" + std::string(p_maxTriggers) +
			   R"(, "removeWhenExhausted": )" + std::string(p_remove) +
			   R"(, "effects": [{"id": "zap", "type": "damage", "applyTo": "primaryUnit",
			   "requiresLivingSource": false, "kind": "magical", "base": 2, "strengthRatioPermille": 0,
			   "magicPowerRatioPermille": 0}]}])";
	};

	expectRejected(
		snare(limited("0", "true")),
		"an unlimited trigger is never exhausted, so it cannot remove the object on exhaustion");
	expectRejected(snare(limited("-1", "false")), "a negative trigger count");
	expectRejected(snare(limited("1000001", "false")), "a trigger count above the maximum");

	EXPECT_NO_THROW((void)parseObject(snare(limited("0", "false"))));
	EXPECT_EQ(parseObject(snare(limited("3", "true"))).triggers[0].maxTriggers, 3U);
}

TEST(BattleObjectDefinitionTest, RejectsATriggerFilterAUnitCanNeverSatisfy)
{
	const auto filtered = [](std::string_view p_filter) {
		return std::string(R"([{"id": "react", "when": "unitEnteredCell", "targetFilter": )") +
			   std::string(p_filter) +
			   R"(, "maxTriggers": 1, "removeWhenExhausted": false, "effects": [{"id": "zap", "type": "damage",
			   "applyTo": "primaryUnit", "requiresLivingSource": false, "kind": "magical", "base": 2,
			   "strengthRatioPermille": 0, "magicPowerRatioPermille": 0}]}])";
	};

	expectRejected(
		snare(filtered(R"({"allowSelf": false, "allowAllies": false, "allowEnemies": false,
			"allowDefeated": false, "allowEmptyCell": true})")),
		"an object triggers on a unit, so an empty cell can never fire it");
	expectRejected(
		snare(filtered(R"({"allowSelf": false, "allowAllies": false, "allowEnemies": true,
			"allowDefeated": true, "allowEmptyCell": false})")),
		"the reserved defeated relation");

	const pg::BattleObjectDefinition friendly = parseObject(snare(filtered(
		R"({"allowSelf": true, "allowAllies": true, "allowEnemies": false,
		   "allowDefeated": false, "allowEmptyCell": false})")));
	EXPECT_TRUE(friendly.triggers[0].targetFilter.allowAllies);
}

TEST(BattleObjectDefinitionTest, RejectsAnObjectWithNoTriggerAndRepeatedIds)
{
	expectRejected(snare("[]"), "an object that never triggers does nothing");

	const std::string effect =
		R"({"id": "zap", "type": "damage", "applyTo": "primaryUnit", "requiresLivingSource": false,
		   "kind": "magical", "base": 2, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})";
	const auto trigger = [&effect](std::string_view p_id, std::string_view p_when, std::string_view p_effectId) {
		std::string body = effect;
		body.replace(body.find("\"zap\""), 5, std::string("\"") + std::string(p_effectId) + "\"");
		return std::string(R"({"id": ")") + std::string(p_id) + R"(", "when": ")" + std::string(p_when) +
			   R"(", "targetFilter": )" + std::string(EnemyOnly) +
			   R"(, "maxTriggers": 1, "removeWhenExhausted": false, "effects": [)" + body + "]}";
	};

	expectRejected(
		snare("[" + trigger("react", "unitEnteredCell", "a") + "," + trigger("react", "unitLeftCell", "b") + "]"),
		"two triggers sharing one id");
	expectRejected(
		snare("[" + trigger("enter", "unitEnteredCell", "zap") + "," + trigger("leave", "unitLeftCell", "zap") + "]"),
		"one effect id reused in a different trigger of the same object");

	// Two triggers with distinct ids on distinct moments are the normal case.
	const pg::BattleObjectDefinition object = parseObject(
		snare("[" + trigger("enter", "unitEnteredCell", "a") + "," + trigger("leave", "unitLeftCell", "b") + "]"));
	ASSERT_EQ(object.triggers.size(), 2U);
	EXPECT_EQ(object.triggers[0].id, "enter") << "authored trigger order is preserved";
	EXPECT_EQ(object.triggers[1].id, "leave");
}

TEST(BattleObjectDefinitionTest, RejectsMissingAndUnknownFields)
{
	for (const std::string_view key :
		 {"displayNameKey", "descriptionKey", "icon", "tags", "blocksMovement", "blocksLineOfSight", "triggers"})
	{
		pgtest::Document document(snare());
		document.value().asObject().erase(std::string(key));
		pg::JsonReader reader = document.reader();
		EXPECT_THROW((void)pg::parseBattleObjectDefinition(reader), pg::JsonError)
			<< "should be rejected: a missing '" << key << "'";
	}

	// The placing effect authors how long an instance lasts; duplicating it here would let the
	// two disagree.
	pgtest::Document extra(snare());
	extra.value()["duration"] = 3;
	pg::JsonReader reader = extra.reader();
	EXPECT_THROW((void)pg::parseBattleObjectDefinition(reader), pg::JsonError);
}
