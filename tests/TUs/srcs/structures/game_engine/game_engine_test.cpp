#include <gtest/gtest.h>

#include "structures/game_engine/spk_game_engine.hpp"

namespace
{
	class ValueComponent : public spk::Component
	{
	public:
		int value = 0;

		ValueComponent() = default;
		explicit ValueComponent(int p_value) :
			value(p_value)
		{
		}
	};

	class SumLogic : public spk::ComponentLogic<ValueComponent>
	{
	public:
		int sum = 0;
		int updateRuns = 0;
		int renderRuns = 0;
		int resizedCount = 0;

	protected:
		void _onUpdateStarted(const spk::UpdateTick &) override
		{
			sum = 0;
		}

		void _parseComponentForUpdate(const spk::UpdateTick &, ValueComponent &p_component) override
		{
			sum += p_component.value;
		}

		void _executeUpdate(const spk::UpdateTick &) override
		{
			++updateRuns;
		}

		void _executeRender(spk::RenderUnitBuilder &) override
		{
			++renderRuns;
		}

		void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &, ValueComponent &) override
		{
			++resizedCount;
		}
	};
}

TEST(GameEngineTest, UpdateDrivesLogicOverRegisteredComponents)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity &entity = engine.addEntity<spk::Entity>();
	entity.addComponent<ValueComponent>(7);

	spk::UpdateTick tick{};
	engine.update(tick);

	EXPECT_EQ(logic.sum, 7);
	EXPECT_EQ(logic.updateRuns, 1);
}

TEST(GameEngineTest, AddReturnsSameLogicInstanceForSameType)
{
	spk::GameEngine engine;

	SumLogic &first = engine.add<SumLogic>();
	SumLogic &second = engine.add<SumLogic>();

	EXPECT_EQ(&first, &second);
	EXPECT_EQ(engine.logic<SumLogic>(), &first);
}

TEST(GameEngineTest, DestroyEntityRemovesItsComponentsFromProcessing)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity &entity = engine.addEntity<spk::Entity>();
	entity.addComponent<ValueComponent>(5);

	spk::UpdateTick tick{};
	engine.update(tick);
	ASSERT_EQ(logic.sum, 5);

	engine.destroyEntity(entity);
	engine.update(tick);

	EXPECT_EQ(logic.sum, 0);
}

TEST(GameEngineTest, ClearEntitiesRemovesEverything)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity &entity = engine.addEntity<spk::Entity>();
	entity.addComponent<ValueComponent>(9);

	engine.clearEntities();

	spk::UpdateTick tick{};
	engine.update(tick);

	EXPECT_EQ(logic.sum, 0);
	EXPECT_TRUE(engine.entities().empty());
}

TEST(GameEngineTest, RenderDrivesRenderHook)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(1);

	spk::RenderUnitBuilder builder;
	engine.render(builder);

	EXPECT_EQ(logic.renderRuns, 1);
}

TEST(GameEngineTest, DispatchEventReachesLogic)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(1);

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	engine.dispatchEvent(event);

	EXPECT_EQ(logic.resizedCount, 1);
}

TEST(GameEngineTest, DeactivatedEngineSkipsUpdate)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(3);

	engine.deactivate();

	spk::UpdateTick tick{};
	engine.update(tick);

	EXPECT_EQ(logic.updateRuns, 0);
}

TEST(GameEngineTest, ComponentAddedAfterEntityIsPickedUpOnNextUpdate)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity &entity = engine.addEntity<spk::Entity>();

	spk::UpdateTick tick{};
	engine.update(tick);
	ASSERT_EQ(logic.sum, 0);

	entity.addComponent<ValueComponent>(4);
	engine.update(tick);

	EXPECT_EQ(logic.sum, 4);
}
