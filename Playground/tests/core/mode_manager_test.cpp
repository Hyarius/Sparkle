#include "battle/battle_context.hpp"
#include "board/board_builder.hpp"
#include "core/mode_manager.hpp"
#include "support/board_fixture.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
	class TrackingMode : public pg::Mode
	{
	private:
		std::string _name;
		std::vector<std::string> &_calls;

	public:
		int updateCount = 0;

		TrackingMode(pg::GameContext &p_context, std::string p_name, std::vector<std::string> &p_calls) :
			pg::Mode(p_context),
			_name(std::move(p_name)),
			_calls(p_calls)
		{
		}

		void enter() override
		{
			_calls.push_back(_name + ".enter");
		}

		void exit() override
		{
			_calls.push_back(_name + ".exit");
		}

		void update(const spk::UpdateTick &) override
		{
			++updateCount;
		}
	};

	struct ManagerFixture
	{
		pg::GameContext context;
		std::vector<std::string> calls;
		TrackingMode *exploration = nullptr;
		TrackingMode *battle = nullptr;
		std::unique_ptr<pg::Mode> explorationOwner;
		std::unique_ptr<pg::Mode> battleOwner;

		ManagerFixture()
		{
			auto explorationMode = std::make_unique<TrackingMode>(context, "exploration", calls);
			exploration = explorationMode.get();
			explorationOwner = std::move(explorationMode);

			auto battleMode = std::make_unique<TrackingMode>(context, "battle", calls);
			battle = battleMode.get();
			battleOwner = std::move(battleMode);
		}
	};
}

TEST(ModeManager, ExitsCurrentModeBeforeEnteringNextMode)
{
	ManagerFixture fixture;
	pg::ModeManager manager(
		fixture.context,
		std::move(fixture.explorationOwner),
		std::move(fixture.battleOwner));

	manager.enterExploration();
	manager.enterBattle();
	manager.enterExploration();

	EXPECT_EQ(
		fixture.calls,
		(std::vector<std::string>{
			"exploration.enter",
			"exploration.exit",
			"battle.enter",
			"battle.exit",
			"exploration.enter"}));
}

TEST(ModeManager, BattleTransitionMethodsAreIdempotent)
{
	ManagerFixture fixture;
	pg::ModeManager manager(
		fixture.context,
		std::move(fixture.explorationOwner),
		std::move(fixture.battleOwner));

	manager.enterExploration();
	manager.enterExploration();
	manager.enterBattle();
	manager.enterBattle();

	EXPECT_EQ(
		fixture.calls,
		(std::vector<std::string>{"exploration.enter", "exploration.exit", "battle.enter"}));
	EXPECT_EQ(manager.currentMode(), fixture.battle);
}

TEST(ModeManager, UpdatesOnlyCurrentMode)
{
	ManagerFixture fixture;
	pg::ModeManager manager(
		fixture.context,
		std::move(fixture.explorationOwner),
		std::move(fixture.battleOwner));
	const spk::UpdateTick tick{};

	manager.update(tick);
	manager.enterExploration();
	manager.update(tick);
	manager.enterBattle();
	manager.update(tick);

	EXPECT_EQ(fixture.exploration->updateCount, 1);
	EXPECT_EQ(fixture.battle->updateCount, 1);
}

TEST(ModeManager, TwoBattleRoundTripsKeepSingleSubscriptionsAndRestoreExploration)
{
	pg::test::BoardFixture fixture({"###", "###", "###"});
	pg::GameContext context;
	pg::BattleContext battle(
		context.events,
		pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));

	EXPECT_EQ(context.events.battleStarted.nbContracts(), 0u);
	{
		pg::ModeManager manager(context);
		EXPECT_EQ(context.events.battleStarted.nbContracts(), 1u);
		EXPECT_EQ(context.events.battleResolved.nbContracts(), 1u);
		EXPECT_EQ(context.events.battleEndConfirmed.nbContracts(), 1u);

		manager.enterExploration();
		for (int encounter = 0; encounter < 2; ++encounter)
		{
			manager.enterBattle();
			context.events.battleResolved.trigger(&battle, pg::BattleSide::Player);
			EXPECT_TRUE(context.world.explorationActive);
			EXPECT_EQ(context.events.battleStarted.nbContracts(), 1u);
			EXPECT_EQ(context.events.battleResolved.nbContracts(), 1u);
			EXPECT_EQ(context.events.battleEndConfirmed.nbContracts(), 1u);
		}
	}
	EXPECT_EQ(context.events.battleStarted.nbContracts(), 0u);
	EXPECT_EQ(context.events.battleResolved.nbContracts(), 0u);
	EXPECT_EQ(context.events.battleEndConfirmed.nbContracts(), 0u);
}
