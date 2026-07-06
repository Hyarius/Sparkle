#include "ai/ai_behaviour.hpp"
#include "ai/ai_evaluator.hpp"
#include "ai/ai_factory.hpp"
#include "ai/conditions/basic_conditions.hpp"
#include "ai/decisions/basic_decisions.hpp"
#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_resolver.hpp"
#include "board/board_builder.hpp"
#include "core/event_center.hpp"
#include "core/json.hpp"
#include "core/registries.hpp"
#include "statuses/status.hpp"
#include "support/board_fixture.hpp"
#include "support/creature_fixture.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	[[nodiscard]] pg::Attributes attributes(int p_health = 20, int p_ap = 6, int p_mp = 5)
	{
		pg::Attributes result;
		result.health = p_health;
		result.ap = p_ap;
		result.mp = p_mp;
		result.stamina = 3.0f;
		result.staminaRate = 1.0f;
		return result;
	}

	[[nodiscard]] pg::Ability melee(std::string p_id = "melee")
	{
		return pg::Ability{
			.id = std::move(p_id),
			.apCost = 2,
			.mpCost = 0,
			.minimumRange = 1,
			.maximumRange = 1,
			.rangeShape = pg::RangeShape::Circle,
			.requiresLineOfSight = true,
			.targetProfile = pg::TargetProfile::Enemy};
	}

	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries loaded;
			loaded.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return loaded;
		}();
		return result;
	}
}

TEST(AIParser, LoadsAuthoredCatalogAndRejectsUnknownFactoryTypes)
{
	ASSERT_EQ(registries().ai().size(), 3);
	const pg::AIBehaviour &healer = registries().ai().get("healer");
	EXPECT_EQ(healer.activeMode, "default");
	ASSERT_EQ(healer.activeRules().size(), 3);

	nlohmann::json json = {{"type", "notACondition"}};
	pg::JsonReader reader(json, "ai-test.json");
	try
	{
		(void)pg::parseAICondition(reader, registries().abilities(), registries().statuses());
		FAIL() << "Expected a JsonError";
	}
	catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.path(), "$.type");
		EXPECT_NE(error.message().find("known types"), std::string::npos);
	}
}

TEST(AIConditions, EvaluateRangeHealthStatusAndCastTruthTables)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = melee();
	pg::BattleUnit &actor = context.addUnit(
		pg::test::creature("actor", attributes(), {&ability}), pg::BattleSide::Enemy);
	pg::BattleUnit &ally = context.addUnit(
		pg::test::creature("ally", attributes()), pg::BattleSide::Enemy);
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("enemy", attributes()), pg::BattleSide::Player);
	ASSERT_TRUE(context.tryPlaceUnit(actor, {2, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(ally, {4, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {1, 0, 0}));

	EXPECT_TRUE(pg::EnemyWithinRangeCondition(1).isMet(actor, context));
	EXPECT_FALSE(pg::EnemyWithinRangeCondition(0).isMet(actor, context));
	EXPECT_FALSE(pg::SelfHpBelowCondition(50).isMet(actor, context));
	actor.attributes.hp.setCurrent(9);
	EXPECT_TRUE(pg::SelfHpBelowCondition(50).isMet(actor, context));
	ally.attributes.hp.setCurrent(9);
	EXPECT_TRUE(pg::AllyHpBelowCondition(50).isMet(actor, context));

	pg::Status marked{.id = "marked"};
	(void)enemy.statuses.add(marked, 1, {});
	EXPECT_TRUE(pg::HasStatusCondition("marked", pg::AIStatusSubject::Enemy).isMet(actor, context));
	EXPECT_TRUE(pg::CanCastCondition(ability).isMet(actor, context));
	actor.attributes.ap.setCurrent(0);
	EXPECT_FALSE(pg::CanCastCondition(ability).isMet(actor, context));
}

TEST(AIDecisions, CastAbilityWalksToNearestLegalCastingCell)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = melee();
	pg::BattleUnit &actor = context.addUnit(
		pg::test::creature("actor-move-cast", attributes(20, 6, 3), {&ability}), pg::BattleSide::Enemy);
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("enemy-move-cast", attributes()), pg::BattleSide::Player);
	ASSERT_TRUE(context.tryPlaceUnit(actor, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {4, 0, 0}));

	pg::CastAbilityDecision decision(ability, pg::AITarget::NearestEnemy);
	std::unique_ptr<pg::BattleAction> action = decision.produce(actor, context);
	ASSERT_NE(action, nullptr);
	ASSERT_EQ(action->kind(), pg::BattleActionKind::Move);
	const auto &move = static_cast<const pg::MoveAction &>(*action);
	EXPECT_EQ(move.destination, spk::Vector3Int(3, 0, 0));
	EXPECT_EQ(move.distance, 3);
}

TEST(AIEvaluator, IllegalDecisionFallsThroughAndAllFailuresEndTurn)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability unknown = melee("unknown");
	pg::BattleUnit &actor = context.addUnit(
		pg::test::creature("actor-evaluator", attributes()), pg::BattleSide::Enemy);
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("enemy-evaluator", attributes()), pg::BattleSide::Player);
	ASSERT_TRUE(context.tryPlaceUnit(actor, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {4, 0, 0}));

	pg::AIBehaviour behaviour{.activeMode = "default"};
	pg::AIRule illegal;
	illegal.decision = std::make_unique<pg::CastAbilityDecision>(unknown, pg::AITarget::NearestEnemy);
	pg::AIRule fallback;
	fallback.decision = std::make_unique<pg::MoveTowardNearestEnemyDecision>();
	behaviour.rulesByMode["default"].push_back(std::move(illegal));
	behaviour.rulesByMode["default"].push_back(std::move(fallback));
	EXPECT_EQ(pg::AIEvaluator::evaluate(behaviour, actor, context)->kind(), pg::BattleActionKind::Move);

	behaviour.rulesByMode["default"].pop_back();
	EXPECT_EQ(pg::AIEvaluator::evaluate(behaviour, actor, context)->kind(), pg::BattleActionKind::EndTurn);
}

TEST(AIEvaluator, AuthoredSkirmisherAttacksThenKites)
{
	pg::test::BoardFixture fixture({"#######"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	const pg::Ability &spark = registries().abilities().get("spark");
	pg::BattleUnit &skirmisher = context.addUnit(
		pg::test::creature("skirmisher", attributes(20, 2, 5), {&spark}), pg::BattleSide::Enemy);
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("kite-target", attributes(100)), pg::BattleSide::Player);
	skirmisher.aiBehaviour = &registries().ai().get("skirmisher");
	ASSERT_TRUE(context.tryPlaceUnit(skirmisher, {2, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {0, 0, 0}));
	context.currentTurn = {.activeUnit = &skirmisher, .turnIndex = 1};

	std::unique_ptr<pg::BattleAction> first = pg::AIEvaluator::evaluate(*skirmisher.aiBehaviour, skirmisher, context);
	ASSERT_EQ(first->kind(), pg::BattleActionKind::Ability);
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, *first));
	std::unique_ptr<pg::BattleAction> second = pg::AIEvaluator::evaluate(*skirmisher.aiBehaviour, skirmisher, context);
	ASSERT_EQ(second->kind(), pg::BattleActionKind::Ability);
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, *second));
	std::unique_ptr<pg::BattleAction> third = pg::AIEvaluator::evaluate(*skirmisher.aiBehaviour, skirmisher, context);
	ASSERT_EQ(third->kind(), pg::BattleActionKind::Move);
	EXPECT_GT(static_cast<const pg::MoveAction &>(*third).destination.x, skirmisher.boardPosition->x);
}
