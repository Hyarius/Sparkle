#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/math_formulas.hpp"
#include "battle/phases/battle_coordinator.hpp"
#include "battle/rules/battle_action_resolver.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "battle/rules/battle_outcome_rules.hpp"
#include "battle/rules/battle_turn_rules.hpp"
#include "core/battle_mode.hpp"
#include "core/event_center.hpp"
#include "core/exploration_mode.hpp"
#include "core/json.hpp"
#include "core/mode_manager.hpp"
#include "support/board_fixture.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace
{
	pg::Attributes attributes(int p_health = 30, float p_stamina = 2.0f)
	{
		return {.health = p_health, .ap = 6, .mp = 3, .attack = 2, .armor = 1, .magic = 1, .resistance = 1, .stamina = p_stamina, .staminaRate = 1.0f};
	}
	pg::Ability tackle(int p_damage = 4)
	{
		return {.id = "tackle", .displayName = "Tackle", .apCost = 2, .minimumRange = 1, .maximumRange = 1, .targetProfile = pg::TargetProfile::Enemy, .baseDamage = p_damage, .damageKind = pg::DamageKind::Physical, .attackRatio = 1.0f};
	}
	std::vector<std::string> eventNames(const pg::BattleContext &p_context)
	{
		std::vector<std::string> result;
		for (const auto &event : p_context.log.events())
		{
			result.emplace_back(pg::battleEventName(event.type));
		}
		return result;
	}
}

TEST(MathFormulas, AppliesPenetrationAndMitigation)
{
	pg::BattleUnit caster({"caster", attributes()}, pg::BattleSide::Player);
	pg::BattleUnit target({"target", attributes()}, pg::BattleSide::Enemy);
	pg::Ability ability = tackle(8);
	caster.attributes.attack = 2;
	target.attributes.armor = 10;
	EXPECT_FLOAT_EQ(pg::MathFormulas::mitigationRatio(10), 0.5f);
	EXPECT_EQ(pg::MathFormulas::computeDamage(caster.attributes, target.attributes, ability), 5);
	caster.attributes.armorPenetration = 10;
	EXPECT_EQ(pg::MathFormulas::computeDamage(caster.attributes, target.attributes, ability), 10);
}

TEST(BattleTurnRules, FasterUnitActsTwiceAndTiesPreferPlayer)
{
	pg::test::BoardFixture fixture({"###"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &fast = context.addUnit({"fast", attributes(30, 2)}, pg::BattleSide::Player);
	pg::BattleUnit &slow = context.addUnit({"slow", attributes(30, 4)}, pg::BattleSide::Enemy);
	pg::BattleTurnRules::advanceToNextReady(context);
	EXPECT_EQ(pg::BattleTurnRules::selectReady(context), &fast);
	pg::BattleTurnRules::beginTurn(context, fast);
	pg::BattleTurnRules::endTurn(context);
	pg::BattleTurnRules::advanceToNextReady(context);
	EXPECT_EQ(pg::BattleTurnRules::selectReady(context), &fast);
	pg::BattleTurnRules::beginTurn(context, fast);
	pg::BattleTurnRules::endTurn(context);
	EXPECT_EQ(pg::BattleTurnRules::selectReady(context), &slow);
	fast.attributes.turnBar.setCurrent(fast.attributes.turnBar.max());
	slow.attributes.turnBar.setCurrent(slow.attributes.turnBar.max());
	EXPECT_EQ(pg::BattleTurnRules::selectReady(context), &fast);
}

TEST(BattleTurnRules, StunPausesFillAndEndTurnRefillsResources)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &unit = context.addUnit({"unit", attributes()}, pg::BattleSide::Player);
	unit.statusTags.push_back("stun");
	pg::BattleTurnRules::advanceTurnBars(context, 10);
	EXPECT_EQ(unit.attributes.turnBar.current(), 0);
	unit.statusTags.clear();
	pg::BattleTurnRules::advanceTurnBars(context, 2);
	pg::BattleTurnRules::beginTurn(context, unit);
	unit.attributes.ap.setCurrent(0);
	unit.attributes.mp.setCurrent(0);
	pg::BattleTurnRules::endTurn(context);
	EXPECT_EQ(unit.attributes.ap.current(), unit.attributes.ap.max());
	EXPECT_EQ(unit.attributes.mp.current(), unit.attributes.mp.max());
}

TEST(BattleActionValidator, ReachabilityExcludesOccupiedCellsAndCircleRangeHonorsProfiles)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = tackle();
	pg::BattleUnit &player = context.addUnit({"player", attributes(), {&ability}}, pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit({"enemy", attributes()}, pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(player, {1, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {2, 0, 0}));
	const auto reachable = pg::BattleActionValidator::getReachableCells(context, player);
	EXPECT_TRUE(reachable.contains({0, 0, 0}));
	EXPECT_FALSE(reachable.contains({2, 0, 0}));
	EXPECT_FALSE(reachable.contains({3, 0, 0}));
	EXPECT_EQ(pg::BattleActionValidator::getValidTargets(context, player, ability), (std::vector<spk::Vector3Int>{{2, 0, 0}}));
}

TEST(BattleTurnRules, CanContinueRequiresMovementOrAffordableAbilityTarget)
{
	pg::test::BoardFixture fixture({"###"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = tackle();
	pg::BattleUnit &player = context.addUnit({"player", attributes(), {&ability}}, pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit({"enemy", attributes()}, pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(player, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {1, 0, 0}));
	context.currentTurn.activeUnit = &player;
	player.attributes.mp.setCurrent(0);
	player.attributes.ap.setCurrent(0);
	EXPECT_FALSE(pg::BattleTurnRules::canContinueTurn(context));
	player.attributes.ap.setCurrent(ability.apCost);
	EXPECT_TRUE(pg::BattleTurnRules::canContinueTurn(context));
	player.attributes.ap.setCurrent(0);
	player.attributes.mp.setCurrent(1);
	EXPECT_FALSE(pg::BattleTurnRules::canContinueTurn(context));
	ASSERT_TRUE(context.tryMoveUnit(enemy, {2, 0, 0}));
	EXPECT_TRUE(pg::BattleTurnRules::canContinueTurn(context));
}

TEST(BattleActionResolver, EmitsGoldenDamageSequenceAndDefeatsTarget)
{
	pg::test::BoardFixture fixture({"##"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = tackle(100);
	pg::BattleUnit &player = context.addUnit({"player", attributes(), {&ability}}, pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit({"enemy", attributes(10)}, pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(player, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, {1, 0, 0}));
	context.currentTurn = {.activeUnit = &player, .turnIndex = 1};
	pg::AbilityAction action(player, ability, {{1, 0, 0}});
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, action));
	EXPECT_TRUE(enemy.isDefeated());
	EXPECT_FALSE(enemy.boardPosition.has_value());
	EXPECT_EQ(eventNames(context), (std::vector<std::string>{"ResourceConsumed", "AbilityCast", "Damage", "UnitDefeated"}));
}

TEST(BattleActionResolver, RejectsMoveWhenOccupancyChangesBeforeResolution)
{
	pg::test::BoardFixture fixture({"###"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &unit = context.addUnit({"unit", attributes()}, pg::BattleSide::Player);
	pg::BattleUnit &blocker = context.addUnit({"blocker", attributes()}, pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(unit, {0, 0, 0}));
	pg::MoveAction action(unit, {1, 0, 0}, 1);
	ASSERT_TRUE(context.tryPlaceUnit(blocker, {1, 0, 0}));
	EXPECT_FALSE(pg::BattleActionResolver::resolve(context, action));
	EXPECT_EQ(unit.boardPosition, spk::Vector3Int(0, 0, 0));
}

TEST(BattleOutcomeRules, ReportsWinnerAndNeutralForSimultaneousDefeat)
{
	pg::test::BoardFixture fixture({"##"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &player = context.addUnit({"player", attributes()}, pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit({"enemy", attributes()}, pg::BattleSide::Enemy);
	enemy.attributes.hp.setCurrent(0);
	EXPECT_EQ(pg::BattleOutcomeRules::winner(context), pg::BattleSide::Player);
	player.attributes.hp.setCurrent(0);
	EXPECT_EQ(pg::BattleOutcomeRules::winner(context), pg::BattleSide::Neutral);
}

TEST(BattleCoordinator, RunsScriptedOneVersusOneThroughSevenPhaseLoop)
{
	pg::test::BoardFixture fixture({"#####", "#####", "#####", "#####", "#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
	pg::Ability ability = tackle(50);
	context.addUnit({"hero", attributes(100, 1), {&ability}}, pg::BattleSide::Player);
	context.addUnit({"enemy", attributes(15, 3), {&ability}}, pg::BattleSide::Enemy);
	pg::BattleCoordinator coordinator(context, 7);
	coordinator.start();
	for (int guard = 0; guard < 30 && !coordinator.finished(); ++guard)
	{
		if (coordinator.currentPhaseName() != "PlayerTurn")
		{
			continue;
		}
		pg::BattleUnit &unit = *context.currentTurn.activeUnit;
		auto targets = pg::BattleActionValidator::getValidTargets(context, unit, ability);
		if (!targets.empty())
		{
			ASSERT_TRUE(coordinator.playerTurnPhase().submitAction(std::make_unique<pg::AbilityAction>(unit, ability, std::vector<spk::Vector3Int>{targets.front()})));
			continue;
		}
		auto enemies = context.getOpponents(unit);
		auto reachable = pg::BattleActionValidator::getReachableCells(context, unit);
		spk::Vector3Int best = *unit.boardPosition;
		int bestDistance = 1000;
		for (const auto &[cell, cost] : reachable)
		{
			if (cost <= 0)
			{
				continue;
			}
			const int d = std::abs(cell.x - enemies.front()->boardPosition->x) + std::abs(cell.z - enemies.front()->boardPosition->z);
			if (d < bestDistance)
			{
				bestDistance = d;
				best = cell;
			}
		}
		if (best != *unit.boardPosition)
		{
			ASSERT_TRUE(coordinator.playerTurnPhase().submitAction(std::make_unique<pg::MoveAction>(unit, best, static_cast<int>(reachable.at(best)))));
		}
		else
		{
			ASSERT_TRUE(coordinator.playerTurnPhase().submitAction(std::make_unique<pg::EndTurnAction>(unit)));
		}
	}
	EXPECT_TRUE(coordinator.finished());
	EXPECT_EQ(coordinator.winner(), pg::BattleSide::Player);
	EXPECT_EQ(coordinator.currentPhaseName(), "End");
	const auto names = eventNames(context);
	EXPECT_TRUE(std::ranges::find(names, "BattleWon") != names.end());
}

TEST(BattleLog, ScriptedTwoVersusTwoSequenceIsStable)
{
	pg::test::BoardFixture fixture({"####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = tackle(100);
	auto &p1 = context.addUnit({"p1", attributes(), {&ability}}, pg::BattleSide::Player);
	auto &p2 = context.addUnit({"p2", attributes(), {&ability}}, pg::BattleSide::Player);
	auto &e1 = context.addUnit({"e1", attributes(), {}}, pg::BattleSide::Enemy);
	auto &e2 = context.addUnit({"e2", attributes(), {}}, pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(p1, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(e1, {1, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(p2, {3, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(e2, {2, 0, 0}));
	context.currentTurn = {.activeUnit = &p1, .turnIndex = 1};
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, pg::AbilityAction(p1, ability, {{1, 0, 0}})));
	context.currentTurn = {.activeUnit = &p2, .turnIndex = 2};
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, pg::AbilityAction(p2, ability, {{2, 0, 0}})));
	EXPECT_EQ(eventNames(context), (std::vector<std::string>{"ResourceConsumed", "AbilityCast", "Damage", "UnitDefeated", "ResourceConsumed", "AbilityCast", "Damage", "UnitDefeated"}));
}

TEST(BattleParsers, ParseAttributesAndAuthoredTackle)
{
	const nlohmann::json json = {{"health", 12}, {"ap", 6}, {"mp", 3}, {"attack", 2}, {"armor", 1}, {"armorPenetration", 0}, {"magic", 3}, {"resistance", 1}, {"resistancePenetration", 0}, {"bonusRange", 0}, {"stamina", 4.0}, {"staminaRate", 1.0}, {"bonusHealing", 0.0}, {"lifeSteal", 0.0}, {"omnivampirism", 0.0}, {"timeEffectResistance", 0.0}};
	pg::JsonReader reader(json, "attributes.json");
	const pg::Attributes parsed = pg::parseAttributes(reader);
	EXPECT_EQ(parsed.health, 12);
	EXPECT_FLOAT_EQ(parsed.stamina, 4.0f);
	const auto abilityJson = pg::JsonLoader::parseFile(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "abilities" / "tackle.json");
	pg::JsonReader abilityReader(abilityJson, "tackle.json");
	const pg::Ability parsedAbility = pg::parseAbility(abilityReader);
	EXPECT_EQ(parsedAbility.displayName, "Tackle");
	EXPECT_EQ(parsedAbility.targetProfile, pg::TargetProfile::Enemy);
}

TEST(ModeManager, BattleEventsEnterAndLeaveBattleMode)
{
	pg::test::BoardFixture fixture({"###", "###", "###"});
	pg::GameContext game;
	pg::ModeManager manager(game);
	manager.enterExploration();
	pg::BattleContext context(game.events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
	context.addUnit({"player", attributes(30, 1)}, pg::BattleSide::Player);
	context.addUnit({"enemy", attributes(30, 3)}, pg::BattleSide::Enemy);
	game.events.battleStarted.trigger(&context);
	EXPECT_NE(dynamic_cast<pg::BattleMode *>(manager.currentMode()), nullptr);
	game.events.battleResolved.trigger(&context, pg::BattleSide::Player);
	EXPECT_NE(dynamic_cast<pg::ExplorationMode *>(manager.currentMode()), nullptr);
}
