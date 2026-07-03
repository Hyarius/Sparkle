#include "battle/battle_context.hpp"
#include "battle/battle_event.hpp"
#include "core/event_center.hpp"
#include "logics/battle_unit_view_logic.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "support/board_fixture.hpp"

#include <gtest/gtest.h>

namespace
{
	[[nodiscard]] pg::BattleUnitSource unitSource(const char *p_name)
	{
		return {p_name, {.health = 10, .ap = 4, .mp = 4, .stamina = 3.0f}, {}};
	}
}

TEST(BattleUnitViewLogic, RegistryTracksPlacementDefeatAndBattleEnd)
{
	pg::test::BoardFixture fixture({"###", "###", "###"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &player = context.addUnit(unitSource("player"), pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit(unitSource("enemy"), pg::BattleSide::Enemy);

	spk::GameEngine engine;
	pg::BattleUnitViewLogic views(engine);
	views.begin(context);
	EXPECT_EQ(views.registeredViewCount(), 0u);
	ASSERT_TRUE(context.tryPlaceUnit(player, fixture.cell(0, 0)));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, fixture.cell(2, 2)));
	EXPECT_EQ(views.registeredViewCount(), 2u);
	EXPECT_EQ(views.activeViewCount(), 2u);

	ASSERT_TRUE(context.defeatUnit(enemy));
	context.report({.type = pg::BattleEventType::UnitDefeated, .target = &enemy});
	EXPECT_EQ(views.registeredViewCount(), 2u);
	EXPECT_EQ(views.activeViewCount(), 1u);
	views.advance(pg::BattleUnitViewLogic::SinkDuration + 0.1f);
	ASSERT_NE(views.find(enemy), nullptr);
	EXPECT_TRUE(views.find(enemy)->hidden);

	views.end();
	EXPECT_EQ(views.registeredViewCount(), 0u);
}

TEST(BattleUnitViewLogic, MoveResolutionStartsAndSettlesPresentationTween)
{
	pg::test::BoardFixture fixture({"###"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
	pg::BattleUnit &unit = context.addUnit(unitSource("unit"), pg::BattleSide::Player);
	const spk::Vector3Int from = fixture.cell(0, 0);
	const spk::Vector3Int to = fixture.cell(2, 0);
	ASSERT_TRUE(context.tryPlaceUnit(unit, from));

	spk::GameEngine engine;
	pg::BattleUnitViewLogic views(engine);
	views.begin(context);
	ASSERT_TRUE(context.tryMoveUnit(unit, to));
	context.report({.type = pg::BattleEventType::DistanceTravelled, .caster = &unit, .distance = 2});
	EXPECT_TRUE(views.viewBusy());

	views.advance(1.0f);
	EXPECT_FALSE(views.viewBusy());
	ASSERT_NE(views.find(unit), nullptr);
	EXPECT_EQ(views.find(unit)->displayedCell, to);
}
