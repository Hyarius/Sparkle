#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_input.hpp"
#include "battle/board_overlay_state.hpp"
#include "battle/phases/battle_coordinator.hpp"
#include "board/board_builder.hpp"
#include "core/event_center.hpp"
#include "support/board_fixture.hpp"
#include "support/creature_fixture.hpp"
#include "ui/ability_bar_widget.hpp"
#include "ui/battle_hud.hpp"
#include "ui/progress_bar_widget.hpp"
#include "ui/turn_order_strip.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{
	pg::Attributes attributes(float p_staminaRate = 1.0f)
	{
		return {
			.health = 30,
			.ap = 6,
			.mp = 3,
			.attack = 2,
			.armor = 1,
			.magic = 1,
			.resistance = 1,
			.stamina = 4.0f,
			.staminaRate = p_staminaRate};
	}

	pg::Ability ability(std::string p_id, int p_ap = 2, int p_minimumRange = 1, int p_maximumRange = 1)
	{
		return {
			.id = std::move(p_id),
			.displayName = "Ability",
			.apCost = p_ap,
			.minimumRange = p_minimumRange,
			.maximumRange = p_maximumRange,
			.targetProfile = pg::TargetProfile::Enemy,
			.baseDamage = 1,
			.damageKind = pg::DamageKind::Physical};
	}
}

TEST(ProgressBarWidget, RebindDoesNotLeakOrDuplicateObservableNotifications)
{
	pg::ObservableResource resource(5, 10);
	pg::ProgressBarWidget bar("Bar");

	bar.bind(resource, "HP");
	const std::size_t firstBind = bar.refreshCount();
	resource.setCurrent(4);
	EXPECT_EQ(bar.refreshCount(), firstBind + 1);

	bar.unbind();
	const std::size_t unbound = bar.refreshCount();
	resource.setCurrent(3);
	EXPECT_EQ(bar.refreshCount(), unbound);

	bar.bind(resource, "HP");
	const std::size_t rebound = bar.refreshCount();
	resource.setCurrent(2);
	EXPECT_EQ(bar.refreshCount(), rebound + 1);
}

TEST(AbilityBarWidget, PageMathUsesEightSlots)
{
	EXPECT_EQ(pg::AbilityBarWidget::pageCountFor(0), 1u);
	EXPECT_EQ(pg::AbilityBarWidget::pageCountFor(8), 1u);
	EXPECT_EQ(pg::AbilityBarWidget::pageCountFor(9), 2u);
	EXPECT_EQ(pg::AbilityBarWidget::pageCountFor(17), 3u);
}

TEST(AbilityBarWidget, SlotsReflectAffordabilityTargetsAndCurrentPage)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	std::vector<pg::Ability> abilities;
	abilities.reserve(9);
	for (int index = 0; index < 9; ++index)
	{
		abilities.push_back(ability("ability-" + std::to_string(index)));
	}
	abilities[1] = ability("unaffordable", 99);
	abilities[2] = ability("no-target", 1, 4, 4);
	std::vector<const pg::Ability *> abilityPointers;
	for (const pg::Ability &entry : abilities) abilityPointers.push_back(&entry);

	pg::BattleUnit &player = context.addUnit(
		pg::test::creature("player", attributes(), abilityPointers), pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("enemy", attributes()), pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(player, fixture.cell(1, 0)));
	ASSERT_TRUE(context.tryPlaceUnit(enemy, fixture.cell(2, 0)));

	pg::AbilityBarWidget bar("Abilities", nullptr);
	bar.bind(&context, &player);
	EXPECT_EQ(bar.pageCount(), 2u);
	EXPECT_TRUE(bar.slot(0).enabled());
	EXPECT_FALSE(bar.slot(1).enabled());
	EXPECT_FALSE(bar.slot(2).enabled());

	bar.setPage(1);
	EXPECT_EQ(bar.slot(0).ability(), &abilities[8]);
	EXPECT_TRUE(bar.slot(0).enabled());
	EXPECT_EQ(bar.slot(1).ability(), nullptr);
	EXPECT_FALSE(bar.slot(1).enabled());
}

TEST(TurnOrderStrip, SortsByComputedTimeAndPrefersPlayerOnTies)
{
	pg::test::BoardFixture fixture({"###"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("enemy", attributes(2.0f)), pg::BattleSide::Enemy);
	pg::BattleUnit &player = context.addUnit(
		pg::test::creature("player", attributes(1.0f)), pg::BattleSide::Player);
	pg::BattleUnit &next = context.addUnit(
		pg::test::creature("next", attributes(1.0f)), pg::BattleSide::Player);
	enemy.attributes.turnBar.setCurrent(2.0f);  // (4 - 2) / 2 = 1
	player.attributes.turnBar.setCurrent(3.0f); // (4 - 3) / 1 = 1
	next.attributes.turnBar.setCurrent(3.5f);   // 0.5

	const std::vector<pg::BattleUnit *> order = pg::TurnOrderStrip::sortedUnits(context);
	ASSERT_EQ(order.size(), 3u);
	EXPECT_EQ(order[0], &next);
	EXPECT_EQ(order[1], &player);
	EXPECT_EQ(order[2], &enemy);
}

TEST(TurnOrderStrip, BindUnbindCycleKeepsOneTurnBarSubscription)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &unit = context.addUnit(
		pg::test::creature("unit", attributes()), pg::BattleSide::Player);
	pg::TurnOrderStrip strip("Turns");

	strip.bind(context);
	const std::size_t firstBind = strip.refreshCount();
	unit.attributes.turnBar.setCurrent(1.0f);
	EXPECT_EQ(strip.refreshCount(), firstBind + 1);

	strip.unbind();
	const std::size_t unbound = strip.refreshCount();
	unit.attributes.turnBar.setCurrent(2.0f);
	EXPECT_EQ(strip.refreshCount(), unbound);

	strip.bind(context);
	const std::size_t rebound = strip.refreshCount();
	unit.attributes.turnBar.setCurrent(3.0f);
	EXPECT_EQ(strip.refreshCount(), rebound + 1);
}

TEST(BattleHud, ActivateDeactivateCycleRebindsCardsExactlyOnce)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::BattleUnit &unit = context.addUnit(
		pg::test::creature("unit", attributes()), pg::BattleSide::Player);
	pg::BattleCoordinator coordinator(context);
	spk::Camera3D camera;
	pg::BoardOverlayState overlay;
	pg::BattleInput input(camera, overlay, [] { return spk::Vector2{800.0f, 600.0f}; });
	input.bind(context, coordinator);
	pg::BattleHud hud("HUD", nullptr);

	hud.bind(context, input, coordinator);
	EXPECT_TRUE(hud.bound());
	EXPECT_TRUE(hud.isActivated());
	const std::size_t firstBind = hud.playerTeam().card(0).hpBar().refreshCount();
	unit.attributes.hp.setCurrent(29);
	EXPECT_EQ(hud.playerTeam().card(0).hpBar().refreshCount(), firstBind + 1);

	hud.unbind();
	EXPECT_FALSE(hud.bound());
	EXPECT_FALSE(hud.isActivated());
	const std::size_t unbound = hud.playerTeam().card(0).hpBar().refreshCount();
	unit.attributes.hp.setCurrent(28);
	EXPECT_EQ(hud.playerTeam().card(0).hpBar().refreshCount(), unbound);

	hud.bind(context, input, coordinator);
	const std::size_t rebound = hud.playerTeam().card(0).hpBar().refreshCount();
	unit.attributes.hp.setCurrent(27);
	EXPECT_EQ(hud.playerTeam().card(0).hpBar().refreshCount(), rebound + 1);
}
