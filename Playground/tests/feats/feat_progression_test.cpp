#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "board/board_builder.hpp"
#include "core/event_center.hpp"
#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_board_service.hpp"
#include "feats/feat_progress.hpp"
#include "support/board_fixture.hpp"

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

	[[nodiscard]] pg::BattleContext makeBattle(pg::EventCenter &p_events)
	{
		pg::test::BoardFixture fixture({"###", "###", "###"});
		return pg::BattleContext(
			p_events,
			pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
	}

	void appendFirstSparkEvents(pg::BattleContext &p_battle, pg::BattleUnit &p_player, pg::BattleUnit &p_enemy)
	{
		p_battle.report(pg::DamageEvent{
			.context = {.turnIndex = 1, .caster = &p_player, .target = &p_enemy},
			.amount = 30,
			.kind = pg::DamageKind::Physical});
		p_battle.report(pg::DamageEvent{
			.context = {.turnIndex = 2, .caster = &p_player, .target = &p_enemy},
			.amount = 30,
			.kind = pg::DamageKind::Physical});
	}
}

TEST(FeatBoardService, LossStillCompletesAndProgressesNewlyReachableNodes)
{
	pg::EventCenter events;
	pg::FeatBoardService service(events);
	pg::CreatureUnit player(registries().creatures().get("sprout"));
	pg::CreatureUnit enemy(registries().creatures().get("sprout"));
	const pg::FeatBoard &board = *player.species->featBoard;
	ASSERT_TRUE(pg::completeNodeDirect(player, namedNode(board, "Steady Growth").uuid));
	pg::BattleContext battle = makeBattle(events);
	pg::BattleUnit &playerBattleUnit = battle.addUnit(player, pg::BattleSide::Player);
	pg::BattleUnit &enemyBattleUnit = battle.addUnit(enemy, pg::BattleSide::Enemy);
	appendFirstSparkEvents(battle, playerBattleUnit, enemyBattleUnit);
	battle.report(pg::HealEvent{
		.context = {.turnIndex = 2, .caster = &playerBattleUnit, .target = &playerBattleUnit},
		.amount = 10});

	pg::CreatureUnit *reportedUnit = nullptr;
	int reportedCompletions = 0;
	auto progressContract = events.featProgressUpdated.subscribe(
		[&](pg::CreatureUnit *p_unit, int p_completions) {
			reportedUnit = p_unit;
			reportedCompletions = p_completions;
		});
	events.battleResolved.trigger(&battle, pg::BattleSide::Enemy);

	const pg::FeatNodeProgress *spark = player.featBoardProgress.findProgress(namedNode(board, "First Spark").uuid);
	const pg::FeatNodeProgress *bloom = player.featBoardProgress.findProgress(namedNode(board, "Bright Bloom").uuid);
	const pg::FeatNodeProgress *form = player.featBoardProgress.findProgress(namedNode(board, "Blaze Bloom").uuid);
	ASSERT_NE(spark, nullptr);
	ASSERT_NE(bloom, nullptr);
	ASSERT_NE(form, nullptr);
	EXPECT_EQ(spark->completionCount, 1);
	ASSERT_EQ(bloom->requirements.size(), 1);
	EXPECT_FLOAT_EQ(bloom->requirements.front().advancement.progress, 40.0f);
	ASSERT_EQ(form->requirements.size(), 1);
	EXPECT_EQ(form->requirements.front().advancement.completedRepeatCount, 0);
	EXPECT_EQ(reportedUnit, &player);
	EXPECT_EQ(reportedCompletions, 1);
	EXPECT_NE(
		std::ranges::find(player.abilities, &registries().abilities().get("ember")),
		player.abilities.end());
	(void)progressContract;
}

TEST(FeatBoardService, CountsOneWinWhenTheBattleLogAlreadyContainsBattleWon)
{
	pg::EventCenter events;
	pg::FeatBoardService service(events);
	pg::CreatureUnit player(registries().creatures().get("sprout"));
	const pg::FeatBoard &board = *player.species->featBoard;
	ASSERT_TRUE(pg::completeNodeDirect(player, namedNode(board, "Steady Growth").uuid));
	pg::BattleContext battle = makeBattle(events);
	pg::BattleUnit &playerBattleUnit = battle.addUnit(player, pg::BattleSide::Player);
	battle.report(pg::BattleWonEvent{
		.context = {.turnIndex = 3, .caster = &playerBattleUnit},
		.side = pg::BattleSide::Player,
		.unitSurvived = true});

	events.battleResolved.trigger(&battle, pg::BattleSide::Player);

	const pg::FeatNodeProgress *form = player.featBoardProgress.findProgress(namedNode(board, "Blaze Bloom").uuid);
	ASSERT_NE(form, nullptr);
	ASSERT_EQ(form->requirements.size(), 1);
	EXPECT_EQ(form->requirements.front().advancement.completedRepeatCount, 1);
}

TEST(FeatBoardService, CascadesCompletionThroughNodesUnlockedByTheSameLog)
{
	pg::EventCenter events;
	pg::FeatBoardService service(events);
	pg::CreatureUnit player(registries().creatures().get("sprout"));
	pg::CreatureUnit enemy(registries().creatures().get("sprout"));
	pg::BattleContext battle = makeBattle(events);
	pg::BattleUnit &playerBattleUnit = battle.addUnit(player, pg::BattleSide::Player);
	pg::BattleUnit &enemyBattleUnit = battle.addUnit(enemy, pg::BattleSide::Enemy);
	appendFirstSparkEvents(battle, playerBattleUnit, enemyBattleUnit);
	battle.report(pg::HealEvent{
		.context = {.turnIndex = 2, .caster = &playerBattleUnit, .target = &playerBattleUnit},
		.amount = 25});

	std::vector<const pg::BattleEvent *> eventPointers;
	for (const pg::BattleEvent &event : battle.log.events())
	{
		eventPointers.push_back(&event);
	}
	EXPECT_EQ(service.registerFightEvents(player, eventPointers, &playerBattleUnit), 2);

	const pg::FeatBoard &board = *player.species->featBoard;
	EXPECT_EQ(player.featBoardProgress.findProgress(namedNode(board, "First Spark").uuid)->completionCount, 1);
	EXPECT_EQ(player.featBoardProgress.findProgress(namedNode(board, "Bright Bloom").uuid)->completionCount, 1);
}

TEST(FeatBoardService, PreservesGameScopeProgressAcrossEvaluations)
{
	pg::EventCenter events;
	pg::FeatBoardService service(events);
	pg::CreatureUnit player(registries().creatures().get("sprout"));
	const pg::FeatBoard &board = *player.species->featBoard;
	ASSERT_TRUE(pg::completeNodeDirect(player, namedNode(board, "First Spark").uuid));
	pg::BattleContext battle = makeBattle(events);
	pg::BattleUnit &playerBattleUnit = battle.addUnit(player, pg::BattleSide::Player);

	pg::BattleEvent firstHeal = pg::HealEvent{
		.context = {.turnIndex = 1, .caster = &playerBattleUnit, .target = &playerBattleUnit},
		.amount = 10};
	const pg::BattleEvent *firstEvents[] = {&firstHeal};
	EXPECT_EQ(service.registerFightEvents(player, firstEvents, &playerBattleUnit), 0);
	const pg::FeatNode &bloomNode = namedNode(board, "Bright Bloom");
	ASSERT_NE(player.featBoardProgress.findProgress(bloomNode.uuid), nullptr);
	EXPECT_FLOAT_EQ(
		player.featBoardProgress.findProgress(bloomNode.uuid)->requirements.front().advancement.progress,
		40.0f);

	pg::BattleEvent secondHeal = pg::HealEvent{
		.context = {.turnIndex = 1, .caster = &playerBattleUnit, .target = &playerBattleUnit},
		.amount = 15};
	const pg::BattleEvent *secondEvents[] = {&secondHeal};
	EXPECT_EQ(service.registerFightEvents(player, secondEvents, &playerBattleUnit), 1);
	EXPECT_EQ(player.featBoardProgress.findProgress(bloomNode.uuid)->completionCount, 1);
}
