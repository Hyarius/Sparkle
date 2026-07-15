#include <gtest/gtest.h>

#include "ai/ai_definition.hpp"
#include "core/combat_definition_validation.hpp"
#include "core/paths.hpp"
#include "core/registries.hpp"
#include "support/json_fixture.hpp"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
	[[nodiscard]] pg::AIBehaviourDefinition parse(std::string_view p_document, std::string_view p_id = "fixture")
	{
		const pgtest::Document document(p_document, std::string(p_id) + ".json");
		pg::JsonReader reader = document.reader();
		pg::AIBehaviourDefinition definition = pg::parseAIBehaviourDefinition(reader);
		definition.id = p_id;
		return definition;
	}

	// The termination fallback every behaviour has to end on.
	constexpr std::string_view Finish = R"({"id": "finish", "conditions": [], "decision": {"type": "endTurn"}})";

	[[nodiscard]] std::string behaviour(std::string_view p_rules)
	{
		return std::string(R"({"version": 1, "displayNameKey": "ai.fixture.name", "rules": [)") +
			   std::string(p_rules) + "]}";
	}

	[[nodiscard]] std::string ruleWith(std::string_view p_conditions, std::string_view p_decision)
	{
		return std::string(R"({"id": "act", "conditions": [)") + std::string(p_conditions) + R"(], "decision": )" +
			   std::string(p_decision) + "}";
	}

	// One rule carrying the condition under test, then the fallback.
	[[nodiscard]] std::string withCondition(std::string_view p_condition)
	{
		return behaviour(
			ruleWith(p_condition, R"({"type": "moveToward", "selector": "nearestEnemy", "maximumMovementPoints": 0})") +
			"," + std::string(Finish));
	}

	[[nodiscard]] std::string withDecision(std::string_view p_decision)
	{
		return behaviour(ruleWith("", p_decision) + "," + std::string(Finish));
	}
}

TEST(AIDefinitionTest, ParsesTheShippedBehaviourAsOrderedIntent)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	ASSERT_TRUE(registries.aiBehaviours().contains("training-aggressive"));
	const pg::AIBehaviourDefinition &aggressive = registries.aiBehaviours().get("training-aggressive");

	ASSERT_EQ(aggressive.rules.size(), 3U);
	EXPECT_EQ(aggressive.rules[0].id, "strike-nearest") << "authored order is evaluation order";
	EXPECT_EQ(aggressive.rules[1].id, "approach");
	EXPECT_EQ(aggressive.rules[2].id, "finish");

	const auto &affordable = std::get<pg::AIAbilityAffordableCondition>(aggressive.rules[0].conditions.at(0));
	EXPECT_EQ(affordable.abilityId, "training-strike");

	// The decision owns its anchor by value: no JSON node, no registry pointer, no callback.
	const auto &cast = std::get<pg::AICastAbilityDecision>(aggressive.rules[0].decision);
	EXPECT_EQ(cast.abilityId, "training-strike");
	EXPECT_EQ(std::get<pg::AIUnitAnchor>(cast.anchor).selector, pg::AIUnitSelector::NearestEnemy);

	EXPECT_TRUE(aggressive.rules[1].conditions.empty()) << "an empty condition list is the AND identity: always";
	EXPECT_TRUE(std::holds_alternative<pg::AIEndTurnDecision>(aggressive.rules[2].decision));
	EXPECT_EQ(pg::aiCastAbilityReferences(aggressive), (std::vector<std::string>{"training-strike"}));
}

TEST(AIDefinitionTest, ParsesEveryConditionAlternative)
{
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "always"})")));
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "healthRatio", "selector": "self", "comparison": "atMost", "permille": 500})")));
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "resourceAtLeast", "resource": "actionPoints", "amount": 2})")));
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "distance", "selector": "nearestEnemy", "comparison": "atMost", "cells": 3})")));
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "hasStatus", "selector": "self", "status": "training-guarded", "present": true})")));
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "hasStatusTag", "selector": "nearestEnemy", "tag": "poison", "present": true})")));
	EXPECT_NO_THROW(auto value = parse(withCondition(R"({"type": "abilityAffordable", "ability": "training-strike"})")));

	// Every selector spells the same way everywhere.
	for (const std::string_view selector :
		 {"self", "nearestEnemy", "nearestAlly", "lowestHealthEnemy", "lowestHealthAlly", "highestHealthEnemy", "highestHealthAlly"})
	{
		EXPECT_NO_THROW(
			auto value = parse(withCondition(
				std::string(R"({"type": "distance", "selector": ")") + std::string(selector) +
				R"(", "comparison": "atLeast", "cells": 1})")))
			<< selector;
	}

	// hasStatus and hasStatusTag build the two distinct alternatives, so a condition can never
	// carry both an id and a tag, or neither.
	const pg::AIBehaviourDefinition byId = parse(withCondition(
		R"({"type": "hasStatus", "selector": "self", "status": "training-guarded", "present": false})"));
	const auto &status = std::get<pg::AIStatusCondition>(byId.rules[0].conditions.at(0));
	EXPECT_FALSE(status.present);
	EXPECT_EQ(std::get<pg::AIStatusIdReference>(status.reference).statusId, "training-guarded");

	const pg::AIBehaviourDefinition byTag = parse(withCondition(
		R"({"type": "hasStatusTag", "selector": "self", "tag": "poison", "present": true})"));
	const auto &tagged = std::get<pg::AIStatusCondition>(byTag.rules[0].conditions.at(0));
	EXPECT_EQ(std::get<pg::AIStatusTagReference>(tagged.reference).tag, "poison");
}

TEST(AIDefinitionTest, ParsesEveryAnchorAndDecisionAlternative)
{
	for (const std::string_view anchor :
		 {R"({"type": "unit", "selector": "lowestHealthAlly"})",
		  R"({"type": "selfCell"})",
		  R"({"type": "bestArea", "preferred": "enemies"})",
		  R"({"type": "bestArea", "preferred": "allies"})",
		  R"({"type": "nearestLegalCellTo", "selector": "nearestEnemy"})"})
	{
		EXPECT_NO_THROW(
			auto value = parse(withDecision(
				std::string(R"({"type": "castAbility", "ability": "training-guard", "anchor": )") +
				std::string(anchor) + "}")))
			<< anchor;
	}

	EXPECT_NO_THROW(auto value = parse(withDecision(R"({"type": "moveAway", "selector": "nearestEnemy", "maximumMovementPoints": 3})")));
	EXPECT_NO_THROW(auto value = parse(withDecision(R"({"type": "endTurn"})")));

	// Zero means "every point currently available"; the cap is otherwise a real number.
	const pg::AIBehaviourDefinition capped = parse(withDecision(
		R"({"type": "moveToward", "selector": "nearestAlly", "maximumMovementPoints": 2})"));
	const auto &move = std::get<pg::AIMoveTowardDecision>(capped.rules[0].decision);
	EXPECT_EQ(move.selector, pg::AIUnitSelector::NearestAlly);
	EXPECT_EQ(move.maximumMovementPoints, 2U);
	EXPECT_THROW(
		auto value = parse(withDecision(
			R"({"type": "moveToward", "selector": "nearestAlly", "maximumMovementPoints": 1000001})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(withDecision(
			R"({"type": "moveToward", "selector": "nearestAlly", "maximumMovementPoints": -1})")),
		pg::JsonError);
}

TEST(AIDefinitionTest, IsStrictAboutUnknownMissingAndOutOfRangeFields)
{
	EXPECT_THROW(auto value = parse(R"({"version": 2, "displayNameKey": "ai.fixture.name", "rules": []})"), pg::JsonError);
	EXPECT_THROW(auto value = parse(behaviour("")), pg::JsonError) << "a behaviour has at least one rule";
	EXPECT_THROW(
		auto value = parse(std::string(R"({"version": 1, "displayNameKey": "ai.fixture.name", "mode": "aggressive",
			"rules": [)") + std::string(Finish) +
						   "]}"),
		pg::JsonError)
		<< "there is no mode dictionary in v1";

	// Unknown members of an alternative, and members of the wrong alternative.
	EXPECT_THROW(
		auto value = parse(withCondition(R"({"type": "always", "selector": "self"})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "hasStatus", "selector": "self", "status": "training-guarded", "tag": "poison",
			   "present": true})")),
		pg::JsonError)
		<< "a status query is either by id or by tag, never both";
	EXPECT_THROW(auto value = parse(withCondition(R"({"type": "utility", "score": 10})")), pg::JsonError)
		<< "no score expression exists in v1";

	// Bounds.
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "healthRatio", "selector": "self", "comparison": "atMost", "permille": 1001})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "distance", "selector": "nearestEnemy", "comparison": "atMost", "cells": 129})")),
		pg::JsonError);
	// A resource amount is a runtime pool integer: above INT_MAX it fails contextually rather than
	// narrowing into a negative.
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "resourceAtLeast", "resource": "actionPoints", "amount": 2147483648})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "resourceAtLeast", "resource": "actionPoints", "amount": -1})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "healthRatio", "selector": "sneakiestEnemy", "comparison": "atMost", "permille": 500})")),
		pg::JsonError);
}

TEST(AIDefinitionTest, RequiresUniqueRulesAnAloneAlwaysAndATerminationFallback)
{
	EXPECT_THROW(
		auto value = parse(behaviour(
			std::string(R"({"id": "act", "conditions": [], "decision": {"type": "endTurn"}},)") +
			R"({"id": "act", "conditions": [], "decision": {"type": "endTurn"}})")),
		pg::JsonError)
		<< "two rules with one id would make persisted diagnostics ambiguous";

	// "always" is the readable spelling of the empty list, so it stands alone.
	EXPECT_THROW(
		auto value = parse(withCondition(
			R"({"type": "always"}, {"type": "distance", "selector": "nearestEnemy", "comparison": "atMost",
			   "cells": 1})")),
		pg::JsonError);

	// The last rule is the termination proof: unconditional, and an endTurn.
	EXPECT_THROW(
		auto value = parse(behaviour(
			R"({"id": "chase", "conditions": [], "decision": {"type": "moveToward", "selector": "nearestEnemy",
			   "maximumMovementPoints": 0}})")),
		pg::JsonError)
		<< "a behaviour that can never end its turn would hold the scheduler forever";
	EXPECT_THROW(
		auto value = parse(behaviour(
			R"({"id": "maybe", "conditions": [{"type": "healthRatio", "selector": "self", "comparison": "atMost",
			   "permille": 100}], "decision": {"type": "endTurn"}})")),
		pg::JsonError)
		<< "a conditional last rule is not a fallback";

	// An explicit "always" endTurn is a fallback, and so is an empty-condition one.
	EXPECT_NO_THROW(auto value = parse(behaviour(R"({"id": "finish", "conditions": [{"type": "always"}], "decision": {"type": "endTurn"}})")));
}

TEST(AIDefinitionTest, ResolvesAbilityStatusAndTagReferencesAgainstTheLoadedGraph)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	const auto validate = [&registries](const pg::AIBehaviourDefinition &p_behaviour) {
		pg::Registry<pg::AIBehaviourDefinition> behaviours;
		behaviours.add(p_behaviour.id, p_behaviour);
		pg::validateAIGraph(registries.statuses(), registries.abilities(), behaviours);
	};

	EXPECT_NO_THROW(validate(parse(withCondition(
		R"({"type": "hasStatus", "selector": "self", "status": "training-guarded", "present": true})"))));
	EXPECT_THROW(
		validate(parse(withCondition(
			R"({"type": "hasStatus", "selector": "self", "status": "ghost-status", "present": true})"))),
		pg::JsonError);
	EXPECT_THROW(
		validate(parse(withCondition(R"({"type": "abilityAffordable", "ability": "ghost-ability"})"))),
		pg::JsonError);
	EXPECT_THROW(
		validate(parse(withDecision(
			R"({"type": "castAbility", "ability": "ghost-ability", "anchor": {"type": "selfCell"}})"))),
		pg::JsonError);

	// An AI tag is a question about the content that exists now: a misspelled one would simply be
	// false forever, which is the bug that never gets reported.
	EXPECT_NO_THROW(validate(parse(withCondition(
		R"({"type": "hasStatusTag", "selector": "self", "tag": "buff", "present": true})"))));
	try
	{
		validate(parse(withCondition(
			R"({"type": "hasStatusTag", "selector": "self", "tag": "poison", "present": true})")));
		FAIL() << "no shipped status carries a 'poison' tag";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(error.message().find("poison"), std::string::npos);
		EXPECT_NE(error.message().find("act"), std::string::npos) << "and it names the rule";
	}
}
