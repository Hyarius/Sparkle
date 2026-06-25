#include <gtest/gtest.h>

#include <string>

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/game_engine/spk_entity.hpp"
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

	class ValueLogic : public spk::ComponentLogic<ValueComponent>
	{
	public:
		std::string trace;
		int sum = 0;
		int renderStartedCount = 0;
		int renderParsedCount = 0;
		int renderExecutedCount = 0;
		int resizedStartedCount = 0;
		int resizedCount = 0;
		int resizedExecutedCount = 0;
		bool consumeResizeOnStart = false;

	protected:
		void _onUpdateStarted(const spk::UpdateTick &) override
		{
			trace += "S";
			sum = 0;
		}

		void _parseComponentForUpdate(const spk::UpdateTick &, ValueComponent &p_component) override
		{
			trace += "P";
			sum += p_component.value;
		}

		void _executeUpdate(const spk::UpdateTick &) override
		{
			trace += "E";
		}

		void _onRenderStarted() override
		{
			++renderStartedCount;
		}

		void _parseComponentForRender(ValueComponent &) override
		{
			++renderParsedCount;
		}

		void _executeRender(spk::RenderUnitBuilder &) override
		{
			++renderExecutedCount;
		}

		void _onWindowResizedEventStarted(spk::WindowResizedEvent &p_event) override
		{
			++resizedStartedCount;

			if (consumeResizeOnStart == true)
			{
				p_event.consume();
			}
		}

		void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &, ValueComponent &) override
		{
			++resizedCount;
		}

		void _executeWindowResizedEvent(spk::WindowResizedEvent &) override
		{
			++resizedExecutedCount;
		}
	};
}

TEST(ComponentLogicTest, UpdateRunsBeginParseEndInOrderOverProcessableComponents)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(2);
	entity.addComponent<ValueComponent>(3);

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	spk::UpdateTick tick{};
	logics.update(tick, engine.componentRegistry());

	EXPECT_EQ(logic.trace, "SPPE");
	EXPECT_EQ(logic.sum, 5);
}

TEST(ComponentLogicTest, RenderRunsBeginParseEndOverProcessableComponents)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(2);
	entity.addComponent<ValueComponent>(3);

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();
	spk::RenderUnitBuilder builder;

	logics.render(builder, engine.componentRegistry());

	EXPECT_EQ(logic.renderStartedCount, 1);
	EXPECT_EQ(logic.renderParsedCount, 2);
	EXPECT_EQ(logic.renderExecutedCount, 1);
}

TEST(ComponentLogicTest, DeactivatedLogicDoesNotRun)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(4);

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();
	logic.deactivate();

	spk::UpdateTick tick{};
	logics.update(tick, engine.componentRegistry());

	EXPECT_TRUE(logic.trace.empty());
}

TEST(ComponentLogicTest, UpdateSkipsNonProcessableComponents)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(9);

	entity.deactivate();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	spk::UpdateTick tick{};
	logics.update(tick, engine.componentRegistry());

	// Begin/end still run, but no component is parsed.
	EXPECT_EQ(logic.trace, "SE");
	EXPECT_EQ(logic.sum, 0);
}

TEST(ComponentLogicTest, EventReachesPerComponentHook)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	logics.dispatchEvent(event, engine.componentRegistry());

	EXPECT_EQ(logic.resizedCount, 1);
}

TEST(ComponentLogicTest, EventConsumedDuringStartSkipsPerComponentHook)
{
	spk::GameEngine engine;
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();
	logic.consumeResizeOnStart = true;

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	logics.dispatchEvent(event, engine.componentRegistry());

	EXPECT_EQ(logic.resizedStartedCount, 1);
	EXPECT_EQ(logic.resizedCount, 0);
	EXPECT_EQ(logic.resizedExecutedCount, 1);
	EXPECT_TRUE(event.isConsumed());
}
