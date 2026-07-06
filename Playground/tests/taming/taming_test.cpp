#include "abilities/effects/effect_types.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_outcome_rules.hpp"
#include "board/board_builder.hpp"
#include "core/game_context.hpp"
#include "core/json.hpp"
#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_progress.hpp"
#include "feats/feat_requirement.hpp"
#include "support/board_fixture.hpp"
#include "taming/taming_condition.hpp"
#include "taming/taming_progress.hpp"
#include "taming/taming_service.hpp"
#include "taming/wild_battle_unit.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries loaded;
			loaded.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return loaded;
		}();
		return result;
	}

	[[nodiscard]] const pg::FeatNode &namedNode(const pg::FeatBoard &p_board, const std::string &p_name)
	{
		const auto found = std::ranges::find_if(p_board.nodes, [&](const pg::FeatNode &p_node) {
			return p_node.displayName == p_name;
		});
		if (found == p_board.nodes.end())
		{
			throw std::logic_error("missing test feat node '" + p_name + "'");
		}
		return *found;
	}

	[[nodiscard]] pg::BattleContext makeBattle(pg::EventCenter &p_events, bool p_allowsTaming = true)
	{
		pg::test::BoardFixture fixture({"###", "###", "###"});
		pg::BattleContext result(
			p_events,
			pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
		result.allowsTaming = p_allowsTaming;
		return result;
	}

	void fulfillSproutProfile(
		pg::BattleContext &p_battle, pg::BattleUnit &p_player, pg::BattleUnit &p_wild)
	{
		p_battle.report(pg::DamageEvent{
			.context = {.turnIndex = 1, .caster = &p_player, .target = &p_wild},
			.amount = 8,
			.kind = pg::DamageKind::Magical});
		p_battle.report(pg::TurnEndedEvent{
			.context = {.turnIndex = 1, .caster = &p_player},
			.nearestUnitDistance = 1});
		p_battle.report(pg::TurnEndedEvent{
			.context = {.turnIndex = 2, .caster = &p_player},
			.nearestUnitDistance = 1});
	}
}

TEST(TamingProfile, LoadsAuthoredConditionsAndKeepsFeatOnlyTypesOutOfItsFactory)
{
	EXPECT_EQ(registries().creatures().get("sprout").tamingProfile.conditions.size(), 2);
	EXPECT_EQ(registries().creatures().get("ember-fox").tamingProfile.conditions.size(), 2);

	nlohmann::json json = {
		{"type", "winBattleCount"},
		{"requireUnitSurvival", true}};
	pg::JsonReader tamingReader(json, "taming-test.json");
	EXPECT_THROW((void)pg::parseTamingCondition(tamingReader), pg::JsonError);
	pg::JsonReader featReader(json, "feat-test.json");
	EXPECT_NE(pg::parseFeatRequirement(featReader), nullptr);
}

TEST(TamingProgress, BecomesImpressedExactlyWhenItsLastConditionCompletes)
{
	const pg::TamingProfile &profile = registries().creatures().get("sprout").tamingProfile;
	pg::TamingProgress progress(profile);
	pg::BattleEvent damage = pg::DamageEvent{.amount = 8, .kind = pg::DamageKind::Magical};
	pg::BattleEvent firstTurn = pg::TurnEndedEvent{.context = {.turnIndex = 1}, .nearestUnitDistance = 1};
	pg::BattleEvent secondTurn = pg::TurnEndedEvent{.context = {.turnIndex = 2}, .nearestUnitDistance = 1};
	const pg::BattleEvent *damageEvents[] = {&damage};
	const pg::BattleEvent *firstTurnEvents[] = {&firstTurn};
	const pg::BattleEvent *secondTurnEvents[] = {&secondTurn};

	progress.evaluateEvents(damageEvents);
	EXPECT_FALSE(progress.isImpressed());
	progress.evaluateEvents(firstTurnEvents);
	EXPECT_FALSE(progress.isImpressed());
	progress.evaluateEvents(secondTurnEvents);
	EXPECT_TRUE(progress.isImpressed());
}

TEST(TamingService, ImpressedLastEnemyLeavesTheBoardAndRecruitsOnVictory)
{
	pg::GameContext game;
	game.newGame(registries());
	pg::TamingService service(game);
	pg::CreatureUnit wildSource(registries().creatures().get("sprout"));
	const pg::FeatBoard &board = *wildSource.species->featBoard;
	ASSERT_TRUE(pg::completeNodeDirect(wildSource, namedNode(board, "Deep Roots").uuid));
	ASSERT_TRUE(pg::completeNodeDirect(wildSource, namedNode(board, "Blaze Bloom").uuid));
	pg::FeatNodeProgress &partial = wildSource.featBoardProgress.getOrCreateProgress(namedNode(board, "Bright Bloom"));
	partial.requirements.front().advancement.progress = 60.0f;

	pg::BattleContext battle = makeBattle(game.events);
	pg::BattleUnit &player = battle.addUnit(*game.player.team[0], pg::BattleSide::Player);
	pg::BattleUnit &enemy = battle.addUnit(wildSource, pg::BattleSide::Enemy);
	auto *wild = dynamic_cast<pg::WildBattleUnit *>(&enemy);
	ASSERT_NE(wild, nullptr);
	ASSERT_TRUE(battle.tryPlaceUnit(player, {0, 0, 0}));
	ASSERT_TRUE(battle.tryPlaceUnit(enemy, {1, 0, 0}));
	pg::BattleUnit *impressed = nullptr;
	pg::CreatureUnit *recruited = nullptr;
	auto impressedContract = game.events.creatureImpressed.subscribe(
		[&](pg::BattleUnit *p_unit) { impressed = p_unit; });
	auto recruitedContract = game.events.creatureRecruited.subscribe(
		[&](pg::CreatureUnit *p_unit) { recruited = p_unit; });
	game.events.battleStarted.trigger(&battle);

	fulfillSproutProfile(battle, player, enemy);

	EXPECT_TRUE(wild->tamingProgress.isImpressed());
	EXPECT_EQ(impressed, &enemy);
	EXPECT_TRUE(enemy.hasLeftBattle);
	EXPECT_FALSE(enemy.boardPosition.has_value());
	EXPECT_EQ(battle.board.tryGetUnitAt({1, 0, 0}), nullptr);
	EXPECT_EQ(pg::BattleOutcomeRules::winner(battle), pg::BattleSide::Player);
	game.events.battleResolved.trigger(&battle, pg::BattleSide::Player);

	ASSERT_NE(recruited, nullptr);
	EXPECT_EQ(game.player.teamSize(), 3);
	EXPECT_EQ(recruited->species, wildSource.species);
	EXPECT_EQ(recruited->currentFormId, "blaze");
	EXPECT_EQ(recruited->attributes.health, recruited->species->attributes.health + 2);
	const pg::FeatNodeProgress *recruitedPartial = recruited->featBoardProgress.findProgress(namedNode(board, "Bright Bloom").uuid);
	EXPECT_TRUE(
		recruitedPartial == nullptr ||
		recruitedPartial->requirements.front().advancement.progress == 0.0f);
	(void)service;
	(void)impressedContract;
	(void)recruitedContract;
}

TEST(TamingService, DefeatFailsTamingPermanently)
{
	pg::GameContext game;
	game.newGame(registries());
	pg::TamingService service(game);
	pg::CreatureUnit wildSource(registries().creatures().get("sprout"));
	pg::BattleContext battle = makeBattle(game.events);
	pg::BattleUnit &player = battle.addUnit(*game.player.team[0], pg::BattleSide::Player);
	pg::BattleUnit &enemy = battle.addUnit(wildSource, pg::BattleSide::Enemy);
	auto *wild = dynamic_cast<pg::WildBattleUnit *>(&enemy);
	ASSERT_NE(wild, nullptr);
	ASSERT_TRUE(battle.tryPlaceUnit(player, {0, 0, 0}));
	ASSERT_TRUE(battle.tryPlaceUnit(enemy, {1, 0, 0}));
	game.events.battleStarted.trigger(&battle);
	battle.report(pg::DamageEvent{
		.context = {.turnIndex = 1, .caster = &player, .target = &enemy},
		.amount = 8,
		.kind = pg::DamageKind::Magical});
	enemy.attributes.hp.setCurrent(0);
	EXPECT_TRUE(battle.defeatUnit(enemy));
	EXPECT_TRUE(wild->tamingProgress.hasFailed());
	battle.report(pg::TurnEndedEvent{.context = {.turnIndex = 1, .caster = &player}, .nearestUnitDistance = 1});
	battle.report(pg::TurnEndedEvent{.context = {.turnIndex = 2, .caster = &player}, .nearestUnitDistance = 1});
	EXPECT_FALSE(wild->tamingProgress.isImpressed());
	(void)service;
}

TEST(TamingService, LossForfeitsImpressedCreatures)
{
	pg::GameContext game;
	game.newGame(registries());
	pg::TamingService service(game);
	pg::CreatureUnit wildSource(registries().creatures().get("sprout"));
	pg::BattleContext battle = makeBattle(game.events);
	pg::BattleUnit &player = battle.addUnit(*game.player.team[0], pg::BattleSide::Player);
	pg::BattleUnit &enemy = battle.addUnit(wildSource, pg::BattleSide::Enemy);
	game.events.battleStarted.trigger(&battle);
	fulfillSproutProfile(battle, player, enemy);
	ASSERT_TRUE(dynamic_cast<pg::WildBattleUnit &>(enemy).tamingProgress.isImpressed());

	game.events.battleResolved.trigger(&battle, pg::BattleSide::Enemy);

	EXPECT_EQ(game.player.teamSize(), 2);
	EXPECT_TRUE(game.player.storage.empty());
	(void)service;
}

TEST(TamingService, FullTeamSendsRecruitToStorage)
{
	pg::GameContext game;
	game.newGame(registries());
	while (game.player.teamSize() < pg::PlayerData::TeamCapacity)
	{
		game.player.addCreatureToTeamOrStorage(
			std::make_unique<pg::CreatureUnit>(registries().creatures().get("sprout")));
	}
	pg::TamingService service(game);
	pg::CreatureUnit wildSource(registries().creatures().get("sprout"));
	pg::BattleContext battle = makeBattle(game.events);
	pg::BattleUnit &player = battle.addUnit(*game.player.team[0], pg::BattleSide::Player);
	pg::BattleUnit &enemy = battle.addUnit(wildSource, pg::BattleSide::Enemy);
	game.events.battleStarted.trigger(&battle);
	fulfillSproutProfile(battle, player, enemy);
	game.events.battleResolved.trigger(&battle, pg::BattleSide::Player);

	EXPECT_EQ(game.player.teamSize(), pg::PlayerData::TeamCapacity);
	EXPECT_EQ(game.player.storage.size(), 1);
	(void)service;
}

TEST(TamingService, DisabledTamingUsesOrdinaryEnemiesAndIgnoresMatchingEvents)
{
	pg::GameContext game;
	game.newGame(registries());
	pg::TamingService service(game);
	pg::CreatureUnit enemySource(registries().creatures().get("sprout"));
	pg::BattleContext battle = makeBattle(game.events, false);
	pg::BattleUnit &player = battle.addUnit(*game.player.team[0], pg::BattleSide::Player);
	pg::BattleUnit &enemy = battle.addUnit(enemySource, pg::BattleSide::Enemy);
	EXPECT_EQ(dynamic_cast<pg::WildBattleUnit *>(&enemy), nullptr);
	int impressedCount = 0;
	auto impressedContract = game.events.creatureImpressed.subscribe(
		[&](pg::BattleUnit *) { ++impressedCount; });
	game.events.battleStarted.trigger(&battle);
	fulfillSproutProfile(battle, player, enemy);

	EXPECT_EQ(impressedCount, 0);
	EXPECT_FALSE(enemy.hasLeftBattle);
	game.events.battleResolved.trigger(&battle, pg::BattleSide::Player);
	EXPECT_EQ(game.player.teamSize(), 2);
	(void)service;
	(void)impressedContract;
}
