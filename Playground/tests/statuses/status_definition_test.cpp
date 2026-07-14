#include <gtest/gtest.h>

#include "statuses/status_definition.hpp"
#include "support/json_fixture.hpp"

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace
{
	[[nodiscard]] pg::StatusDefinition parseStatus(std::string_view p_content)
	{
		const pgtest::Document document(p_content);
		pg::JsonReader reader = document.reader();
		return pg::parseStatusDefinition(reader);
	}

	void expectRejected(std::string_view p_content, std::string_view p_because)
	{
		EXPECT_THROW((void)parseStatus(p_content), pg::JsonError) << "should be rejected: " << p_because;
	}

	// The shipped Training Guarded, which every case below varies one block of.
	[[nodiscard]] std::string guarded(
		std::string_view p_tags = R"(["buff", "guard"])",
		std::string_view p_stacking = R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
		std::string_view p_modifiers = R"([{"id": "armor-bonus", "stat": "armor", "operation": "add",
			"value": 3, "perStack": false}])",
		std::string_view p_hooks = "[]")
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "status.training-guarded.name",
			"descriptionKey": "status.training-guarded.description",
			"icon": [1, 0],
			"tags": )") +
			   std::string(p_tags) + R"(, "stacking": )" + std::string(p_stacking) + R"(, "modifiers": )" +
			   std::string(p_modifiers) + R"(, "hooks": )" + std::string(p_hooks) + "}";
	}
}

TEST(StatusDefinitionTest, ParsesTheGoldenDefinition)
{
	const pg::StatusDefinition status = parseStatus(guarded());

	EXPECT_EQ(status.displayNameKey, "status.training-guarded.name") << "a definition carries a key, not a sentence";
	EXPECT_EQ(status.descriptionKey, "status.training-guarded.description");
	EXPECT_EQ(status.icon, spk::Vector2Int(1, 0));
	EXPECT_EQ(status.tags, (std::vector<std::string>{"buff", "guard"}));
	EXPECT_EQ(status.maxStacks, 1U);
	EXPECT_EQ(status.reapplyPolicy, pg::StatusReapplyPolicy::ReplaceStacks);
	EXPECT_EQ(status.durationRefreshPolicy, pg::DurationRefreshPolicy::Replace);
	EXPECT_TRUE(status.hooks.empty());
	EXPECT_FALSE(pg::isStunStatus(status));

	ASSERT_EQ(status.modifiers.size(), 1U);
	EXPECT_EQ(status.modifiers[0].id, "armor-bonus");
	EXPECT_EQ(status.modifiers[0].stat, pg::CreatureStat::Armor);
	EXPECT_EQ(status.modifiers[0].operation, pg::StatModifierOperation::Add);
	EXPECT_EQ(status.modifiers[0].value, 3);
	EXPECT_FALSE(status.modifiers[0].perStack);
}

TEST(StatusDefinitionTest, ReadsEveryStackingAndDurationRefreshPolicy)
{
	const auto stacked = [](std::string_view p_reapply, std::string_view p_refresh) {
		return std::string(R"({"maxStacks": 5, "reapply": ")") + std::string(p_reapply) +
			   R"(", "durationRefresh": ")" + std::string(p_refresh) + R"("})";
	};

	EXPECT_EQ(
		parseStatus(guarded(R"(["buff"])", stacked("addStacks", "extend"))).reapplyPolicy,
		pg::StatusReapplyPolicy::AddStacks);
	EXPECT_EQ(
		parseStatus(guarded(R"(["buff"])", stacked("addStacks", "extend"))).durationRefreshPolicy,
		pg::DurationRefreshPolicy::Extend);
	EXPECT_EQ(
		parseStatus(guarded(R"(["buff"])", stacked("replaceStacks", "keepLonger"))).durationRefreshPolicy,
		pg::DurationRefreshPolicy::KeepLonger);
	EXPECT_EQ(parseStatus(guarded(R"(["buff"])", stacked("addStacks", "replace"))).maxStacks, 5U);

	expectRejected(guarded(R"(["buff"])", stacked("refreshStacks", "replace")), "an unknown reapply policy");
	expectRejected(guarded(R"(["buff"])", stacked("addStacks", "reset")), "an unknown duration refresh policy");
	expectRejected(
		guarded(R"(["buff"])", R"({"maxStacks": 0, "reapply": "addStacks", "durationRefresh": "replace"})"),
		"a status that can never hold a stack");
	expectRejected(
		guarded(R"(["buff"])", R"({"maxStacks": 1000001, "reapply": "addStacks", "durationRefresh": "replace"})"),
		"more stacks than the maximum");
	expectRejected(
		guarded(R"(["buff"])", R"({"maxStacks": 1, "reapply": "addStacks"})"),
		"a stacking block missing its duration refresh policy");
}

TEST(StatusDefinitionTest, AuthorsAStaminaAddInMillisecondsAndEveryOtherStatInItsOwnUnit)
{
	const pg::StatusDefinition hasted = parseStatus(guarded(
		R"(["buff"])",
		R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
		R"([{"id": "faster", "stat": "stamina", "operation": "add", "valueMilliseconds": -500,
		   "perStack": false}])"));
	ASSERT_EQ(hasted.modifiers.size(), 1U);
	EXPECT_EQ(hasted.modifiers[0].stat, pg::CreatureStat::Stamina);
	EXPECT_EQ(hasted.modifiers[0].value, -500) << "lower stamina is faster";

	expectRejected(
		guarded(
			R"(["buff"])",
			R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
			R"([{"id": "faster", "stat": "stamina", "operation": "add", "value": -500, "perStack": false}])"),
		"an additive stamina modifier authored in the unitless 'value' field");

	expectRejected(
		guarded(
			R"(["buff"])",
			R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
			R"([{"id": "armor-bonus", "stat": "armor", "operation": "add", "valueMilliseconds": 3,
			   "perStack": false}])"),
		"a non-stamina modifier authored in milliseconds");

	// A stamina scale is still a plain permille factor, so it uses "value".
	const pg::StatusDefinition scaled = parseStatus(guarded(
		R"(["buff"])",
		R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
		R"([{"id": "slower", "stat": "stamina", "operation": "multiplyPermille", "value": 1200,
		   "perStack": false}])"));
	EXPECT_EQ(scaled.modifiers[0].operation, pg::StatModifierOperation::MultiplyPermille);
	EXPECT_EQ(scaled.modifiers[0].value, 1200);
}

TEST(StatusDefinitionTest, RejectsModifierValuesOutsideTheirBoundsAndDuplicateIds)
{
	const auto modified = [](std::string_view p_modifiers) {
		return guarded(
			R"(["buff"])",
			R"({"maxStacks": 3, "reapply": "addStacks", "durationRefresh": "replace"})",
			p_modifiers);
	};

	expectRejected(
		modified(R"([{"id": "armor-bonus", "stat": "armor", "operation": "add", "value": 0, "perStack": false}])"),
		"an additive modifier of zero");
	expectRejected(
		modified(
			R"([{"id": "armor-bonus", "stat": "armor", "operation": "add", "value": 1000001, "perStack": false}])"),
		"an additive modifier above the maximum");
	expectRejected(
		modified(R"([{"id": "scale", "stat": "armor", "operation": "multiplyPermille", "value": 0,
		   "perStack": false}])"),
		"a permille factor of zero");
	expectRejected(
		modified(R"([{"id": "scale", "stat": "armor", "operation": "multiplyPermille", "value": 10001,
		   "perStack": false}])"),
		"a permille factor above the maximum");
	expectRejected(
		modified(R"([{"id": "scale", "stat": "luck", "operation": "add", "value": 1, "perStack": false}])"),
		"a stat that does not exist");
	expectRejected(
		modified(R"([
			{"id": "armor-bonus", "stat": "armor", "operation": "add", "value": 3, "perStack": false},
			{"id": "armor-bonus", "stat": "resistance", "operation": "add", "value": 3, "perStack": false}
		])"),
		"two modifiers sharing one id");

	const pg::StatusDefinition perStack = parseStatus(
		modified(R"([{"id": "armor-bonus", "stat": "armor", "operation": "add", "value": 2, "perStack": true}])"));
	EXPECT_TRUE(perStack.modifiers[0].perStack);
}

TEST(StatusDefinitionTest, ReadsEveryHookMomentThroughTheWhenKey)
{
	const auto hooked = [](std::string_view p_when, std::string_view p_id) {
		return std::string(R"([{"id": ")") + std::string(p_id) + R"(", "when": ")" + std::string(p_when) +
			   R"(", "effects": [{"id": "tick-)" + std::string(p_id) +
			   R"(", "type": "changeResource", "applyTo": "primaryUnit", "requiresLivingSource": false,
			   "resource": "actionPoints", "delta": 1}]}])";
	};

	const std::array<std::pair<std::string_view, pg::StatusHook>, 10> moments{
		std::pair{std::string_view("applied"), pg::StatusHook::Applied},
		std::pair{std::string_view("activationStart"), pg::StatusHook::ActivationStart},
		std::pair{std::string_view("activationEnd"), pg::StatusHook::ActivationEnd},
		std::pair{std::string_view("afterAbilityCast"), pg::StatusHook::AfterAbilityCast},
		std::pair{std::string_view("afterDamageDealt"), pg::StatusHook::AfterDamageDealt},
		std::pair{std::string_view("afterDamageTaken"), pg::StatusHook::AfterDamageTaken},
		std::pair{std::string_view("afterHealingDone"), pg::StatusHook::AfterHealingDone},
		std::pair{std::string_view("afterHealingReceived"), pg::StatusHook::AfterHealingReceived},
		std::pair{std::string_view("afterVoluntaryMove"), pg::StatusHook::AfterVoluntaryMove},
		std::pair{std::string_view("removed"), pg::StatusHook::Removed}};

	for (const auto &[literal, hook] : moments)
	{
		const pg::StatusDefinition status = parseStatus(guarded(
			R"(["buff"])",
			R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
			"[]",
			hooked(literal, "react")));
		ASSERT_EQ(status.hooks.size(), 1U) << literal;
		EXPECT_EQ(status.hooks[0].hook, hook) << literal;
		EXPECT_EQ(status.hooks[0].id, "react");
		ASSERT_EQ(status.hooks[0].effects.size(), 1U);
		EXPECT_TRUE(std::holds_alternative<pg::ChangeResourceEffectSpec>(status.hooks[0].effects[0].payload));
	}

	// "hook" is the C++ field name; the authored key is "when".
	expectRejected(
		guarded(
			R"(["buff"])",
			R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
			"[]",
			R"([{"id": "react", "hook": "applied", "effects": [{"id": "tick", "type": "changeResource",
			   "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints",
			   "delta": 1}]}])"),
		"a hook authored with the C++ field name instead of 'when'");
}

TEST(StatusDefinitionTest, RejectsRepeatedHooksIdsAndEffectIdsAcrossHooks)
{
	const auto withHooks = [](std::string_view p_hooks) {
		return guarded(
			R"(["buff"])",
			R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
			"[]",
			p_hooks);
	};

	expectRejected(
		withHooks(R"([
			{"id": "first", "when": "activationStart", "effects": [{"id": "a", "type": "changeResource",
			 "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints", "delta": 1}]},
			{"id": "second", "when": "activationStart", "effects": [{"id": "b", "type": "changeResource",
			 "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints", "delta": 1}]}
		])"),
		"two entries for one moment: their relative order would be undefined");

	expectRejected(
		withHooks(R"([
			{"id": "react", "when": "activationStart", "effects": [{"id": "a", "type": "changeResource",
			 "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints", "delta": 1}]},
			{"id": "react", "when": "activationEnd", "effects": [{"id": "b", "type": "changeResource",
			 "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints", "delta": 1}]}
		])"),
		"two hooks sharing one id");

	expectRejected(
		withHooks(R"([
			{"id": "first", "when": "activationStart", "effects": [{"id": "tick", "type": "changeResource",
			 "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints", "delta": 1}]},
			{"id": "second", "when": "activationEnd", "effects": [{"id": "tick", "type": "changeResource",
			 "applyTo": "primaryUnit", "requiresLivingSource": false, "resource": "actionPoints", "delta": -1}]}
		])"),
		"one effect id reused in a different hook of the same status");

	expectRejected(
		withHooks(R"([{"id": "react", "when": "applied", "effects": []}])"),
		"a hook that runs no effect");
}

TEST(StatusDefinitionTest, RejectsMalformedTagsAndUnknownFields)
{
	expectRejected(guarded(R"(["buff", "buff"])"), "a repeated tag");
	expectRejected(guarded(R"(["Buff"])"), "a tag outside the content-id grammar");
	expectRejected(guarded(R"("buff")"), "a tag list that is not an array");

	// A status with no tags at all is fine.
	EXPECT_TRUE(parseStatus(guarded("[]")).tags.empty());

	pgtest::Document extra(guarded());
	extra.value()["durationTurns"] = -1;
	pg::JsonReader reader = extra.reader();
	EXPECT_THROW((void)pg::parseStatusDefinition(reader), pg::JsonError)
		<< "a status carries no duration: the applying effect authors it";
}

TEST(StatusDefinitionTest, RecognizesTheReservedStunTag)
{
	const pg::StatusDefinition stun = parseStatus(guarded(R"(["debuff", "stun"])",
		R"({"maxStacks": 1, "reapply": "replaceStacks", "durationRefresh": "replace"})",
		"[]"));
	EXPECT_TRUE(pg::isStunStatus(stun));
	// The stun restriction itself is a property of every applyStatus reference to it, so it is
	// enforced by the cross-validator once all statuses are known.
}
