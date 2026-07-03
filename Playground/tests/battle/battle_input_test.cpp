#include "abilities/ability.hpp"
#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_input.hpp"
#include "battle/board_overlay_state.hpp"
#include "board/board_builder.hpp"
#include "core/event_center.hpp"
#include "support/board_fixture.hpp"
#include "support/creature_fixture.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace
{
	using pg::OverlayCategory;

	pg::Attributes attributes()
	{
		return {.health = 30, .ap = 6, .mp = 3, .attack = 2, .armor = 1, .magic = 1, .resistance = 1, .stamina = 2.0f, .staminaRate = 1.0f};
	}
	pg::Ability tackle()
	{
		return {.id = "tackle", .displayName = "Tackle", .apCost = 2, .minimumRange = 1, .maximumRange = 1, .targetProfile = pg::TargetProfile::Enemy, .baseDamage = 4, .damageKind = pg::DamageKind::Physical, .attackRatio = 1.0f};
	}

	struct InputScenario
	{
		pg::test::BoardFixture fixture{{"#####", "#####", "#####", "#####", "#####"}};
		pg::EventCenter events;
		pg::Ability ability = tackle();
		pg::BattleContext context{events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry())};
		pg::BoardOverlayState overlay;

		InputScenario()
		{
			pg::BattleUnit &player = context.addUnit(pg::test::creature("player", attributes(), {&ability}), pg::BattleSide::Player);
			pg::BattleUnit &enemy = context.addUnit(pg::test::creature("enemy", attributes()), pg::BattleSide::Enemy);
			EXPECT_TRUE(context.tryPlaceUnit(player, fixture.cell(2, 2)));
			EXPECT_TRUE(context.tryPlaceUnit(enemy, fixture.cell(3, 2)));
			context.currentTurn.activeUnit = &player;
		}
	};
}

TEST(BattleInputController, MoveModeHoverBuildsRangeAndPathAndClickReturnsMove)
{
	InputScenario scene;
	pg::BattleInputController controller(scene.context, scene.overlay);

	EXPECT_EQ(controller.mode(), pg::BattleInputController::Mode::Move);
	controller.hover(scene.fixture.cell(2, 3));
	EXPECT_FALSE(scene.overlay.cells(OverlayCategory::MoveRange).empty());
	EXPECT_FALSE(scene.overlay.cells(OverlayCategory::Path).empty());
	EXPECT_EQ(scene.overlay.cells(OverlayCategory::Hovered).size(), 1u);

	std::unique_ptr<pg::BattleAction> action = controller.click(scene.fixture.cell(2, 3));
	ASSERT_NE(action, nullptr);
	ASSERT_EQ(action->kind(), pg::BattleActionKind::Move);
	EXPECT_EQ(static_cast<pg::MoveAction &>(*action).destination, scene.fixture.cell(2, 3));
}

TEST(BattleInputController, FirstInteractionClearsDeploymentPreview)
{
	InputScenario scene;
	scene.overlay.set(
		OverlayCategory::Deployment,
		{scene.fixture.cell(0, 0), scene.fixture.cell(1, 0)});
	pg::BattleInputController controller(scene.context, scene.overlay);

	controller.hover(scene.fixture.cell(2, 3));

	EXPECT_TRUE(scene.overlay.cells(OverlayCategory::Deployment).empty());
}

TEST(BattleInputController, OutOfReachMoveClickIsRejectedAndFlaggedInvalid)
{
	InputScenario scene;
	pg::BattleInputController controller(scene.context, scene.overlay);

	// {0,0,0} is 4 steps from the unit which only has 3 MP.
	std::unique_ptr<pg::BattleAction> action = controller.click(scene.fixture.cell(0, 0));
	EXPECT_EQ(action, nullptr);
	EXPECT_EQ(scene.overlay.cells(OverlayCategory::Invalid).size(), 1u);
}

TEST(BattleInputController, AbilityModeTargetsEnemyAndEscapeReturnsToMove)
{
	InputScenario scene;
	pg::BattleInputController controller(scene.context, scene.overlay);

	controller.handleDigit(1);
	EXPECT_EQ(controller.mode(), pg::BattleInputController::Mode::Ability);
	EXPECT_EQ(controller.selectedAbility(), &scene.ability);
	EXPECT_FALSE(scene.overlay.cells(OverlayCategory::AbilityRange).empty());

	controller.hover(scene.fixture.cell(3, 2)); // the adjacent enemy
	EXPECT_EQ(scene.overlay.cells(OverlayCategory::AreaOfEffect).size(), 1u);

	std::unique_ptr<pg::BattleAction> action = controller.click(scene.fixture.cell(3, 2));
	ASSERT_NE(action, nullptr);
	ASSERT_EQ(action->kind(), pg::BattleActionKind::Ability);
	ASSERT_EQ(static_cast<pg::AbilityAction &>(*action).targetCells.size(), 1u);
	EXPECT_EQ(static_cast<pg::AbilityAction &>(*action).targetCells.front(), scene.fixture.cell(3, 2));

	controller.escape();
	EXPECT_EQ(controller.mode(), pg::BattleInputController::Mode::Move);
}

TEST(BattleInputController, AbilityClickOnAnEmptyCellIsRejected)
{
	InputScenario scene;
	pg::BattleInputController controller(scene.context, scene.overlay);

	controller.handleDigit(1);
	std::unique_ptr<pg::BattleAction> action = controller.click(scene.fixture.cell(2, 3)); // empty, in range
	EXPECT_EQ(action, nullptr);
	EXPECT_EQ(scene.overlay.cells(OverlayCategory::Invalid).size(), 1u);
}

TEST(BattleInputController, EndTurnIntentBuildsAnEndTurnActionForTheActiveUnit)
{
	InputScenario scene;
	pg::BattleInputController controller(scene.context, scene.overlay);

	std::unique_ptr<pg::BattleAction> action = controller.endTurn();
	ASSERT_NE(action, nullptr);
	EXPECT_EQ(action->kind(), pg::BattleActionKind::EndTurn);
	EXPECT_EQ(&action->source, scene.context.currentTurn.activeUnit);
}
