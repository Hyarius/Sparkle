#include <gtest/gtest.h>

#include "conditions/condition_definition.hpp"
#include "support/json_fixture.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
	// The four window keys every leaf must author, so a case can vary one of them without
	// restating the rest.
	constexpr std::string_view FightWindow =
		R"("window": "fight", "windowActor": "any", "requiredWindowCount": 1, "windowMode": "cumulative")";

	constexpr std::string_view DamageBody =
		R"("actor": "subject", "role": "source", "counterpart": "opponentTeam", "kind": "magical",
		   "component": "health", "targetHadShield": "any", "sourceAbilities": [],
		   "aggregation": "maximum", "comparison": "greaterThan", "threshold": 80)";

	constexpr std::string_view HealingBody =
		R"("actor": "subject", "role": "source", "counterpart": "subjectAllies", "sourceAbilities": [],
		   "aggregation": "sum", "comparison": "atLeast", "threshold": 30)";

	constexpr std::string_view AbilityCastBody =
		R"("actor": "opponentTeam", "abilities": [], "sameAbility": true, "affected": "any",
		   "minimumAffectedUnits": 0, "minimumTargetDistance": 0, "maximumTargetDistance": 64,
		   "comparison": "atLeast", "threshold": 3)";

	constexpr std::string_view StatusBody =
		R"("actor": "subject", "role": "source", "counterpart": "subjectAllies", "action": "removed",
		   "statuses": [], "tags": ["poison"], "aggregation": "count", "comparison": "atLeast",
		   "threshold": 2)";

	constexpr std::string_view ShieldBody =
		R"("actor": "subject", "role": "target", "counterpart": "any", "action": "absorbed",
		   "kind": "physical", "aggregation": "sum", "comparison": "atLeast", "threshold": 40)";

	constexpr std::string_view MovementBody =
		R"("actor": "subject", "movement": "voluntary", "direction": "any", "aggregation": "sum",
		   "comparison": "atLeast", "threshold": 10)";

	constexpr std::string_view ResourceBody =
		R"("actor": "subject", "action": "spent", "resource": "actionPoints", "reasons": ["abilityCost"],
		   "aggregation": "sum", "comparison": "atLeast", "threshold": 12)";

	constexpr std::string_view PositionBody =
		R"("sample": "afterMove", "actor": "opponentTeam", "relativeTo": "subject", "comparison": "atMost",
		   "distance": 1)";

	constexpr std::string_view UnitRemovalBody =
		R"("removed": "opponentTeam", "creditedTo": "subject", "reason": "defeated", "comparison": "atLeast",
		   "threshold": 3)";

	constexpr std::string_view BattleOutcomeBody =
		R"("outcome": "subjectTeamVictory", "requireSubjectActive": true)";

	constexpr std::string_view SurvivorCountBody =
		R"("units": "opponentTeam", "state": "active", "comparison": "atLeast", "threshold": 2)";

	[[nodiscard]] std::string leaf(
		std::string_view p_type,
		std::string_view p_body,
		std::string_view p_window = FightWindow,
		std::string_view p_id = "c")
	{
		return std::string(R"({"id": ")") + std::string(p_id) + R"(", "type": ")" + std::string(p_type) +
			   R"(", "descriptionKey": "c.description", )" + std::string(p_window) + ", " + std::string(p_body) + "}";
	}

	[[nodiscard]] pg::ConditionSpec parse(std::string_view p_json, pg::ConditionParseState &p_state)
	{
		const pgtest::Document document(p_json, "board.json");
		pg::JsonReader reader = document.reader();
		return pg::parseConditionSpec(reader, p_state);
	}

	[[nodiscard]] pg::ConditionSpec parse(std::string_view p_json)
	{
		pg::ConditionParseState state;
		return parse(p_json, state);
	}

	// By value: the caller usually parses into a temporary, and a reference into it would dangle
	// at the end of the full expression.
	template <typename TPayload>
	[[nodiscard]] TPayload payloadOf(const pg::ConditionSpec &p_condition)
	{
		return std::get<TPayload>(p_condition.payload);
	}
}

TEST(ConditionDefinitionTest, ParsesEveryAmountProducingLeaf)
{
	const pg::ConditionSpec damage = parse(leaf("damage", DamageBody, R"("window": "ability", "windowActor": "subject",
		"requiredWindowCount": 1, "windowMode": "cumulative")"));
	const auto &damageSpec = payloadOf<pg::DamageConditionSpec>(damage);
	EXPECT_EQ(damage.id, "c");
	EXPECT_EQ(damage.descriptionKey, "c.description");
	EXPECT_EQ(damageSpec.leaf.window, pg::ConditionWindow::Ability);
	EXPECT_EQ(damageSpec.leaf.windowActor, pg::ConditionUnitSet::Subject);
	EXPECT_EQ(damageSpec.leaf.requiredWindowCount, 1U);
	EXPECT_EQ(damageSpec.leaf.windowMode, pg::WindowCountMode::Cumulative);
	EXPECT_EQ(damageSpec.actor, pg::ConditionUnitSet::Subject);
	EXPECT_EQ(damageSpec.role, pg::ConditionEventRole::Source);
	EXPECT_EQ(damageSpec.counterpart, pg::ConditionUnitSet::OpponentTeam);
	EXPECT_EQ(damageSpec.kind, pg::DamageKindFilter::Magical);
	EXPECT_EQ(damageSpec.component, pg::DamageComponent::Health);
	EXPECT_EQ(damageSpec.targetHadShield, pg::PresenceFilter::Any);
	EXPECT_TRUE(damageSpec.sourceAbilities.empty()) << "an empty filter means any ability";
	EXPECT_EQ(damageSpec.metric.aggregation, pg::ConditionAggregation::Maximum);
	EXPECT_EQ(damageSpec.metric.comparison, pg::ConditionComparison::GreaterThan);
	EXPECT_EQ(damageSpec.metric.threshold, 80);
	EXPECT_FALSE(damageSpec.metric.maximum.has_value());

	const auto &healing = payloadOf<pg::HealingConditionSpec>(parse(leaf("healing", HealingBody)));
	EXPECT_EQ(healing.counterpart, pg::ConditionUnitSet::SubjectAllies);
	EXPECT_EQ(healing.metric.aggregation, pg::ConditionAggregation::Sum);

	const auto &status = payloadOf<pg::StatusConditionSpec>(parse(leaf("status", StatusBody)));
	EXPECT_EQ(status.action, pg::StatusConditionAction::Removed);
	EXPECT_EQ(status.tags, (std::vector<std::string>{"poison"}));
	EXPECT_TRUE(status.statuses.empty());

	const auto &shield = payloadOf<pg::ShieldConditionSpec>(parse(leaf("shield", ShieldBody)));
	EXPECT_EQ(shield.action, pg::ShieldConditionAction::Absorbed);
	EXPECT_EQ(shield.kind, pg::DamageKindFilter::Physical);
	EXPECT_EQ(shield.role, pg::ConditionEventRole::Target);

	const auto &movement = payloadOf<pg::MovementConditionSpec>(parse(leaf("movement", MovementBody)));
	EXPECT_EQ(movement.movement, pg::MovementFilter::Voluntary);
	EXPECT_EQ(movement.direction, pg::MovementDirection::Any);

	const auto &resource = payloadOf<pg::ResourceConditionSpec>(parse(leaf("resource", ResourceBody)));
	EXPECT_EQ(resource.action, pg::ResourceConditionAction::Spent);
	EXPECT_EQ(resource.resource, pg::BattleResource::ActionPoints);
	EXPECT_EQ(resource.reasons, (std::vector<pg::ResourceChangeReason>{pg::ResourceChangeReason::AbilityCost}));
}

TEST(ConditionDefinitionTest, ParsesEveryDirectComparisonLeaf)
{
	const auto &cast = payloadOf<pg::AbilityCastConditionSpec>(parse(leaf("abilityCast", AbilityCastBody)));
	EXPECT_EQ(cast.actor, pg::ConditionUnitSet::OpponentTeam);
	EXPECT_TRUE(cast.sameAbility) << "one deterministic per-ability bucket has to meet the comparison";
	EXPECT_EQ(cast.affected, pg::ConditionUnitSet::Any);
	EXPECT_EQ(cast.minimumAffectedUnits, 0U);
	EXPECT_EQ(cast.maximumTargetDistance, 64U);
	EXPECT_EQ(cast.threshold, 3);

	const auto &position = payloadOf<pg::PositionConditionSpec>(parse(leaf(
		"position",
		PositionBody,
		R"("window": "turn", "windowActor": "opponentTeam", "requiredWindowCount": 1, "windowMode": "cumulative")")));
	EXPECT_EQ(position.leaf.windowActor, pg::ConditionUnitSet::OpponentTeam)
		<< "only an opponent's activation opens an eligible window";
	EXPECT_EQ(position.sample, pg::PositionSample::AfterMove);
	EXPECT_EQ(position.relativeTo, pg::ConditionUnitSet::Subject);
	EXPECT_EQ(position.distance, 1U);
	EXPECT_FALSE(position.maximumDistance.has_value());

	const auto &removal = payloadOf<pg::UnitRemovalConditionSpec>(parse(leaf("unitRemoval", UnitRemovalBody)));
	EXPECT_EQ(removal.removed, pg::ConditionUnitSet::OpponentTeam);
	EXPECT_EQ(removal.creditedTo, pg::ConditionUnitSet::Subject);
	EXPECT_EQ(removal.reason, pg::UnitRemovalReasonFilter::Defeated);
	EXPECT_EQ(removal.threshold, 3);

	const auto &outcome = payloadOf<pg::BattleOutcomeConditionSpec>(parse(leaf("battleOutcome", BattleOutcomeBody)));
	EXPECT_EQ(outcome.outcome, pg::RelativeBattleOutcome::SubjectTeamVictory);
	EXPECT_TRUE(outcome.requireSubjectActive);

	const auto &survivors = payloadOf<pg::SurvivorCountConditionSpec>(parse(leaf("survivorCount", SurvivorCountBody)));
	EXPECT_EQ(survivors.units, pg::ConditionUnitSet::OpponentTeam);
	EXPECT_EQ(survivors.state, pg::SurvivorState::Active);
	EXPECT_EQ(survivors.threshold, 2U);
}

TEST(ConditionDefinitionTest, ParsesEveryEventAbsencePredicate)
{
	const pg::ConditionSpec streak = parse(leaf(
		"eventAbsence",
		R"("predicate": {"type": "damage", "actor": "subject", "role": "target", "counterpart": "any",
		   "kind": "physical", "component": "health", "sourceAbilities": []})",
		R"("window": "turn", "windowActor": "subject", "requiredWindowCount": 2, "windowMode": "consecutive")"));

	const auto &absence = payloadOf<pg::EventAbsenceConditionSpec>(streak);
	EXPECT_EQ(absence.leaf.requiredWindowCount, 2U);
	EXPECT_EQ(absence.leaf.windowMode, pg::WindowCountMode::Consecutive)
		<< "an eligible window that fails resets the streak to zero";

	const auto &damage = std::get<pg::DamageAbsencePredicate>(absence.predicate);
	EXPECT_EQ(damage.role, pg::ConditionEventRole::Target);
	EXPECT_EQ(damage.kind, pg::DamageKindFilter::Physical);
	EXPECT_EQ(damage.component, pg::DamageComponent::Health);

	const auto &cast = payloadOf<pg::EventAbsenceConditionSpec>(parse(leaf(
		"eventAbsence",
		R"("predicate": {"type": "abilityCast", "actor": "subject", "abilities": ["training-strike"]})")));
	EXPECT_EQ(
		std::get<pg::AbilityCastAbsencePredicate>(cast.predicate).abilities,
		(std::vector<std::string>{"training-strike"}));

	const auto &status = payloadOf<pg::EventAbsenceConditionSpec>(parse(leaf(
		"eventAbsence",
		R"("predicate": {"type": "status", "actor": "subject", "role": "target", "counterpart": "any",
		   "action": "applied", "statuses": [], "tags": ["poison"]})")));
	EXPECT_EQ(
		std::get<pg::StatusAbsencePredicate>(status.predicate).action,
		pg::StatusConditionAction::Applied);

	const auto &movement = payloadOf<pg::EventAbsenceConditionSpec>(
		parse(leaf("eventAbsence", R"("predicate": {"type": "movement", "actor": "subject", "movement": "voluntary"})")));
	EXPECT_EQ(std::get<pg::MovementAbsencePredicate>(movement.predicate).movement, pg::MovementFilter::Voluntary);

	// A predicate holds filters and nothing else: it is not a general negation operator.
	EXPECT_THROW(
		auto value = parse(leaf("eventAbsence", R"("predicate": {"type": "position", "actor": "subject"})")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"eventAbsence",
			R"("predicate": {"type": "movement", "actor": "subject", "movement": "voluntary", "threshold": 3})")),
		pg::JsonError);
}

TEST(ConditionDefinitionTest, ComposesNestedAllOfAndAnyOf)
{
	const std::string json = std::string(R"({
		"id": "support-or-defense",
		"type": "anyOf",
		"descriptionKey": "c.description",
		"children": [
			)") + leaf("healing", HealingBody, FightWindow, "child-a") + "," +
						 leaf("shield", ShieldBody, FightWindow, "child-b") + "]}";

	const pg::ConditionSpec composite = parse(json);
	const auto &anyOf = payloadOf<pg::AnyOfConditionSpec>(composite);
	ASSERT_EQ(anyOf.children.size(), 2U);
	EXPECT_EQ(anyOf.children[0].id, "child-a") << "authored order is preserved";
	EXPECT_EQ(anyOf.children[1].id, "child-b");
	EXPECT_TRUE(std::holds_alternative<pg::HealingConditionSpec>(anyOf.children[0].payload));

	// A composite of one is the child itself; a composite of none completes on nothing.
	const std::string tooFew = std::string(R"({"id": "solo", "type": "allOf", "descriptionKey": "c.description",
		"children": [)") + leaf("healing", HealingBody, FightWindow, "only") + "]}";
	EXPECT_THROW(auto value = parse(tooFew), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(R"({"id": "none", "type": "allOf", "descriptionKey": "c.description", "children": []})"),
		pg::JsonError);

	// A composite carries no window of its own.
	const std::string windowed = std::string(R"({"id": "w", "type": "allOf", "descriptionKey": "c.description",
		"window": "fight", "children": [)") +
								 leaf("healing", HealingBody, FightWindow, "child-a") + "," +
								 leaf("shield", ShieldBody, FightWindow, "child-b") + "]}";
	EXPECT_THROW(auto value = parse(windowed), pg::JsonError);
}

TEST(ConditionDefinitionTest, RejectsCompositesBeyondTheConfiguredDepth)
{
	const auto nest = [](std::size_t p_depth) {
		std::string json = leaf("healing", HealingBody, FightWindow, "innermost");
		for (std::size_t level = p_depth; level >= 1; --level)
		{
			// Two children, because a composite needs at least two: only the first keeps nesting.
			json = std::string(R"({"id": "level-)") + std::to_string(level) +
				   R"(", "type": "allOf", "descriptionKey": "c.description", "children": [)" + json + "," +
				   leaf("shield", ShieldBody, FightWindow, "leaf-" + std::to_string(level)) + "]}";
		}
		return json;
	};

	pg::ConditionParseState atLimit;
	atLimit.limits.maxDepth = 4;
	EXPECT_NO_THROW(auto value = parse(nest(3), atLimit)) << "three composites plus their leaves is exactly four levels";

	pg::ConditionParseState beyond;
	beyond.limits.maxDepth = 4;
	EXPECT_THROW(auto value = parse(nest(4), beyond), pg::JsonError) << "the depth limit is what keeps recursion off the stack";
}

TEST(ConditionDefinitionTest, EnforcesTheWindowAndCountContract)
{
	// A fight or game window has no opening action to filter.
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			HealingBody,
			R"("window": "fight", "windowActor": "subject", "requiredWindowCount": 1, "windowMode": "cumulative")")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			HealingBody,
			R"("window": "game", "windowActor": "subject", "requiredWindowCount": 1, "windowMode": "cumulative")")),
		pg::JsonError);

	// An ability or turn window is opened by one actor's action, so it may filter on it.
	EXPECT_NO_THROW(auto value = parse(leaf(
		"healing",
		HealingBody,
		R"("window": "ability", "windowActor": "subjectTeam", "requiredWindowCount": 5, "windowMode": "consecutive")")));
	EXPECT_NO_THROW(auto value = parse(leaf(
		"healing",
		HealingBody,
		R"("window": "turn", "windowActor": "any", "requiredWindowCount": 3, "windowMode": "cumulative")")));

	// A game window is one continuous persisted window.
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			HealingBody,
			R"("window": "game", "windowActor": "any", "requiredWindowCount": 2, "windowMode": "cumulative")")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			HealingBody,
			R"("window": "game", "windowActor": "any", "requiredWindowCount": 1, "windowMode": "consecutive")")),
		pg::JsonError);

	// Every header key is required: an initializer is not a schema default.
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			HealingBody,
			R"("window": "fight", "requiredWindowCount": 1, "windowMode": "cumulative")")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf("healing", HealingBody, R"("window": "fight", "windowActor": "any", "windowMode": "cumulative")")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			HealingBody,
			R"("window": "fight", "windowActor": "any", "requiredWindowCount": 0, "windowMode": "cumulative")")),
		pg::JsonError);
}

TEST(ConditionDefinitionTest, EnforcesTheMetricContract)
{
	const auto &between = payloadOf<pg::DamageConditionSpec>(parse(leaf(
		"damage",
		R"("actor": "subject", "role": "source", "counterpart": "any", "kind": "any", "component": "total",
		   "targetHadShield": "required", "sourceAbilities": [], "aggregation": "sum", "comparison": "between",
		   "threshold": 10, "maximum": 20)")));
	ASSERT_TRUE(between.metric.maximum.has_value());
	EXPECT_EQ(*between.metric.maximum, 20);
	EXPECT_EQ(between.targetHadShield, pg::PresenceFilter::Required);

	// between needs its upper bound; nothing else may carry one.
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			R"("actor": "subject", "role": "source", "counterpart": "any", "sourceAbilities": [],
			   "aggregation": "sum", "comparison": "between", "threshold": 10)")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			R"("actor": "subject", "role": "source", "counterpart": "any", "sourceAbilities": [],
			   "aggregation": "sum", "comparison": "atLeast", "threshold": 10, "maximum": 20)")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			R"("actor": "subject", "role": "source", "counterpart": "any", "sourceAbilities": [],
			   "aggregation": "sum", "comparison": "between", "threshold": 20, "maximum": 10)")),
		pg::JsonError);

	// Negative thresholds are rejected for the initial catalog.
	EXPECT_THROW(
		auto value = parse(leaf(
			"healing",
			R"("actor": "subject", "role": "source", "counterpart": "any", "sourceAbilities": [],
			   "aggregation": "sum", "comparison": "atLeast", "threshold": -1)")),
		pg::JsonError);

	// A leaf with one direct threshold has no second operand to bound.
	EXPECT_THROW(
		auto value = parse(leaf(
			"unitRemoval",
			R"("removed": "any", "creditedTo": "any", "reason": "any", "comparison": "between", "threshold": 1)")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(leaf(
			"survivorCount",
			R"("units": "any", "state": "active", "comparison": "between", "threshold": 1)")),
		pg::JsonError);

	// A broken shield reports no amount, so only its occurrences can be counted.
	EXPECT_NO_THROW(auto value = parse(leaf(
		"shield",
		R"("actor": "subject", "role": "target", "counterpart": "any", "action": "broken", "kind": "any",
		   "aggregation": "count", "comparison": "atLeast", "threshold": 1)")));
	EXPECT_THROW(
		auto value = parse(leaf(
			"shield",
			R"("actor": "subject", "role": "target", "counterpart": "any", "action": "broken", "kind": "any",
			   "aggregation": "sum", "comparison": "atLeast", "threshold": 1)")),
		pg::JsonError);
}

TEST(ConditionDefinitionTest, EnforcesEveryResourceActionReasonCombination)
{
	const auto resource = [](std::string_view p_action, std::string_view p_reasons) {
		return leaf(
			"resource",
			std::string(R"("actor": "subject", "action": ")") + std::string(p_action) +
				R"(", "resource": "movementPoints", "reasons": )" + std::string(p_reasons) +
				R"(, "aggregation": "count", "comparison": "atLeast", "threshold": 1)");
	};

	EXPECT_NO_THROW(auto value = parse(resource("spent", R"(["abilityCost", "movementCost"])")));
	EXPECT_NO_THROW(auto value = parse(resource("gained", R"(["effect", "activationRefill"])")));
	EXPECT_NO_THROW(
		auto value = parse(resource("lost", R"(["effect", "nextActivationPenaltyConsumption", "effectiveMaximumClamp"])")));
	EXPECT_NO_THROW(auto value = parse(resource("nextActivationPenalty", R"(["effect"])")));

	// The filter is what stops an authored "gain AP from an effect" being farmed by the refill.
	EXPECT_THROW(auto value = parse(resource("gained", R"(["abilityCost"])")), pg::JsonError);
	EXPECT_THROW(auto value = parse(resource("spent", R"(["effect"])")), pg::JsonError);
	EXPECT_THROW(auto value = parse(resource("lost", R"(["activationRefill"])")), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(resource("nextActivationPenalty", R"(["nextActivationPenaltyConsumption"])")),
		pg::JsonError)
		<< "the leaf consumes the applied accumulation event, not its later consumption";

	EXPECT_THROW(auto value = parse(resource("spent", "[]")), pg::JsonError);
	EXPECT_THROW(auto value = parse(resource("spent", R"(["abilityCost", "abilityCost"])")), pg::JsonError);
	EXPECT_THROW(auto value = parse(resource("spent", R"(["ambientDecay"])")), pg::JsonError);
}

TEST(ConditionDefinitionTest, EnforcesTheAbilityCastFilters)
{
	const auto cast = [](std::string_view p_body) { return leaf("abilityCast", p_body); };

	// A filtered affected set with a zero minimum would accept a cast that affected nobody in it.
	EXPECT_THROW(
		auto value = parse(cast(R"("actor": "subject", "abilities": [], "sameAbility": false, "affected": "opponentTeam",
			"minimumAffectedUnits": 0, "minimumTargetDistance": 0, "maximumTargetDistance": 8,
			"comparison": "atLeast", "threshold": 1)")),
		pg::JsonError);
	EXPECT_NO_THROW(
		auto value = parse(cast(R"("actor": "subject", "abilities": [], "sameAbility": false, "affected": "opponentTeam",
			"minimumAffectedUnits": 2, "minimumTargetDistance": 0, "maximumTargetDistance": 8,
			"comparison": "atLeast", "threshold": 1)")));

	// Both teams at most.
	EXPECT_THROW(
		auto value = parse(cast(R"("actor": "subject", "abilities": [], "sameAbility": false, "affected": "any",
			"minimumAffectedUnits": 13, "minimumTargetDistance": 0, "maximumTargetDistance": 8,
			"comparison": "atLeast", "threshold": 1)")),
		pg::JsonError);

	// The distance band is inclusive and ordered.
	EXPECT_THROW(
		auto value = parse(cast(R"("actor": "subject", "abilities": [], "sameAbility": false, "affected": "any",
			"minimumAffectedUnits": 0, "minimumTargetDistance": 5, "maximumTargetDistance": 4,
			"comparison": "atLeast", "threshold": 1)")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(cast(R"("actor": "subject", "abilities": [], "sameAbility": false, "affected": "any",
			"minimumAffectedUnits": 0, "minimumTargetDistance": 0, "maximumTargetDistance": 129,
			"comparison": "atLeast", "threshold": 1)")),
		pg::JsonError);

	// Filters keep authored order and drop repeats.
	const auto &deduplicated = payloadOf<pg::AbilityCastConditionSpec>(
		parse(cast(R"("actor": "subject", "abilities": ["training-strike", "training-guard", "training-strike"],
			"sameAbility": false, "affected": "any", "minimumAffectedUnits": 0, "minimumTargetDistance": 0,
			"maximumTargetDistance": 8, "comparison": "atLeast", "threshold": 1)")));
	EXPECT_EQ(deduplicated.abilities, (std::vector<std::string>{"training-strike", "training-guard"}));

	EXPECT_THROW(
		auto value = parse(cast(R"("actor": "subject", "abilities": ["Training_Strike"], "sameAbility": false, "affected": "any",
			"minimumAffectedUnits": 0, "minimumTargetDistance": 0, "maximumTargetDistance": 8,
			"comparison": "atLeast", "threshold": 1)")),
		pg::JsonError)
		<< "filters name content ids";
}

TEST(ConditionDefinitionTest, EnforcesThePositionAndMovementContracts)
{
	const auto position = [](std::string_view p_body, std::string_view p_window = FightWindow) {
		return leaf("position", p_body, p_window);
	};

	const auto &band = payloadOf<pg::PositionConditionSpec>(parse(position(
		R"("sample": "turnStart", "actor": "subject", "relativeTo": "opponentTeam", "comparison": "between",
		   "distance": 2, "maximumDistance": 5)")));
	ASSERT_TRUE(band.maximumDistance.has_value());
	EXPECT_EQ(*band.maximumDistance, 5U);

	EXPECT_THROW(
		auto value = parse(position(R"("sample": "turnStart", "actor": "subject", "relativeTo": "opponentTeam",
			"comparison": "between", "distance": 2)")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(position(R"("sample": "turnStart", "actor": "subject", "relativeTo": "opponentTeam",
			"comparison": "atMost", "distance": 2, "maximumDistance": 5)")),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(position(R"("sample": "turnStart", "actor": "subject", "relativeTo": "opponentTeam",
			"comparison": "greaterThan", "distance": 2)")),
		pg::JsonError)
		<< "a cell distance is compared with atMost, atLeast, equal or between";
	EXPECT_THROW(
		auto value = parse(position(R"("sample": "turnStart", "actor": "subject", "relativeTo": "opponentTeam",
			"comparison": "atMost", "distance": 129)")),
		pg::JsonError);

	// A whole-window invariant needs a window that opens and closes.
	EXPECT_NO_THROW(auto value = parse(position(
		R"("sample": "throughoutWindow", "actor": "subject", "relativeTo": "opponentTeam", "comparison": "atLeast",
		   "distance": 3)")));
	EXPECT_THROW(
		auto value = parse(position(
			R"("sample": "throughoutWindow", "actor": "subject", "relativeTo": "opponentTeam",
			   "comparison": "atLeast", "distance": 3)",
			R"("window": "game", "windowActor": "any", "requiredWindowCount": 1, "windowMode": "cumulative")")),
		pg::JsonError);

	// Only a displacement carries a direction relative to its source.
	const auto movement = [](std::string_view p_movement, std::string_view p_direction) {
		return leaf(
			"movement",
			std::string(R"("actor": "subject", "movement": ")") + std::string(p_movement) + R"(", "direction": ")" +
				std::string(p_direction) +
				R"(", "aggregation": "sum", "comparison": "atLeast", "threshold": 3)");
	};
	EXPECT_NO_THROW(auto value = parse(movement("displacement", "towardSource")));
	EXPECT_NO_THROW(auto value = parse(movement("any", "awayFromSource")));
	EXPECT_NO_THROW(auto value = parse(movement("teleport", "any")));
	EXPECT_THROW(auto value = parse(movement("voluntary", "towardSource")), pg::JsonError);
	EXPECT_THROW(auto value = parse(movement("teleport", "awayFromSource")), pg::JsonError);
}

TEST(ConditionDefinitionTest, PinsTheSummaryLeavesToTheFightWindow)
{
	const auto gameWindow = R"("window": "game", "windowActor": "any", "requiredWindowCount": 1,
		"windowMode": "cumulative")";

	EXPECT_THROW(auto value = parse(leaf("battleOutcome", BattleOutcomeBody, gameWindow)), pg::JsonError);
	EXPECT_THROW(auto value = parse(leaf("survivorCount", SurvivorCountBody, gameWindow)), pg::JsonError);

	// A survivor count can never demand more units than both teams can hold.
	EXPECT_THROW(
		auto value = parse(leaf("survivorCount", R"("units": "any", "state": "notDefeated", "comparison": "atLeast",
			"threshold": 13)")),
		pg::JsonError);
}

TEST(ConditionDefinitionTest, RejectsUnknownStructureAndDuplicateIds)
{
	EXPECT_THROW(auto value = parse(leaf("teleportation", DamageBody)), pg::JsonError) << "unknown condition type";
	EXPECT_THROW(
		auto value = parse(leaf(
			"damage",
			R"("actor": "subject", "role": "source", "counterpart": "any", "kind": "elemental", "component": "total",
			   "targetHadShield": "any", "sourceAbilities": [], "aggregation": "sum", "comparison": "atLeast",
			   "threshold": 1)")),
		pg::JsonError)
		<< "unknown enum literal";
	EXPECT_THROW(
		auto value = parse(std::string(R"({"id": "c", "type": "healing", "descriptionKey": "c.description", )") +
			  std::string(FightWindow) + ", " + std::string(HealingBody) + R"(, "repeatLimit": 3})"),
		pg::JsonError)
		<< "every condition object forbids unknown keys";
	EXPECT_THROW(auto value = parse(leaf("healing", HealingBody, FightWindow, "Not An Id")), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(std::string(R"({"type": "healing", "descriptionKey": "c.description", )") + std::string(FightWindow) +
			  ", " + std::string(HealingBody) + "}"),
		pg::JsonError)
		<< "a condition without an id has no key for persisted progress";

	// Ids are unique across the whole owning definition, not merely among siblings.
	const std::string reusedNested = std::string(R"({"id": "twice", "type": "allOf",
		"descriptionKey": "c.description", "children": [)") +
									 leaf("healing", HealingBody, FightWindow, "twice") + "," +
									 leaf("shield", ShieldBody, FightWindow, "other") + "]}";
	EXPECT_THROW(auto value = parse(reusedNested), pg::JsonError);

	pg::ConditionParseState shared;
	EXPECT_NO_THROW(auto value = parse(leaf("healing", HealingBody, FightWindow, "same"), shared));
	EXPECT_THROW(auto value = parse(leaf("shield", ShieldBody, FightWindow, "same"), shared), pg::JsonError)
		<< "two nodes of one board cannot reuse a condition id";
}

TEST(ConditionDefinitionTest, ReportsTheFileAndPathOfTheOffendingValue)
{
	const std::string json = std::string(R"({"id": "root", "type": "allOf", "descriptionKey": "c.description",
		"children": [)") + leaf("healing", HealingBody, FightWindow, "child-a") + "," +
						 leaf("shield", R"("actor": "subject", "role": "target", "counterpart": "any",
							"action": "broken", "kind": "any", "aggregation": "sum", "comparison": "atLeast",
							"threshold": 1)", FightWindow, "child-b") +
						 "]}";

	try
	{
		auto value = parse(json);
		FAIL() << "a broken shield cannot be summed";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file().filename(), std::filesystem::path("board.json"));
		EXPECT_EQ(error.path(), "$.children[1].aggregation") << "the path survives the recursion";
	}
}

TEST(ConditionDefinitionTest, CollectsEveryCombatReferenceWithItsAuthoredPath)
{
	const std::string json = std::string(R"({"id": "root", "type": "allOf", "descriptionKey": "c.description",
		"children": [)") +
						 leaf(
							 "damage",
							 R"("actor": "subject", "role": "source", "counterpart": "any", "kind": "any",
								"component": "total", "targetHadShield": "any",
								"sourceAbilities": ["training-strike"], "aggregation": "sum",
								"comparison": "atLeast", "threshold": 1)",
							 FightWindow,
							 "child-a") +
						 "," +
						 leaf(
							 "status",
							 R"("actor": "subject", "role": "target", "counterpart": "any", "action": "applied",
								"statuses": ["training-guarded"], "tags": [], "aggregation": "count",
								"comparison": "atLeast", "threshold": 1)",
							 FightWindow,
							 "child-b") +
						 "]}";

	pg::ConditionReferences references;
	pg::collectConditionReferences(std::vector<pg::ConditionSpec>{parse(json)}, references);

	ASSERT_EQ(references.abilities.size(), 1U);
	EXPECT_EQ(references.abilities[0].id, "training-strike");
	EXPECT_EQ(references.abilities[0].source.jsonPath, "$.children[0]");
	ASSERT_EQ(references.statuses.size(), 1U);
	EXPECT_EQ(references.statuses[0].id, "training-guarded");
	EXPECT_EQ(references.statuses[0].source.jsonPath, "$.children[1]");
}
