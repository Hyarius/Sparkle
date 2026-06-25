#include <gtest/gtest.h>

#include <string>

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/game_engine/spk_entity.hpp"

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
		int synchronizedCount = 0;
		int synchronizedSum = 0;
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

		void _onSynchronizationStarted() override
		{
			synchronizedSum = 0;
		}

		void _parseComponentForSynchronization(ValueComponent &p_component) override
		{
			synchronizedSum += p_component.value;
		}

		void _executeSynchronization() override
		{
			++synchronizedCount;
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
	spk::Entity entity;
	entity.addComponent<ValueComponent>(2);
	entity.addComponent<ValueComponent>(3);

	spk::ComponentRegistry components;
	for (const auto &component : entity.components())
	{
		components.add(component.get());
	}
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	spk::UpdateTick tick{};
	logics.update(tick, components);

	EXPECT_EQ(logic.trace, "SPPE");
	EXPECT_EQ(logic.sum, 5);
}

TEST(ComponentLogicTest, SynchronizeRunsBeginParseEndOverProcessableComponents)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(2);
	entity.addComponent<ValueComponent>(3);

	spk::ComponentRegistry components;
	for (const auto &component : entity.components())
	{
		components.add(component.get());
	}
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	logics.synchronize(components);

	EXPECT_EQ(logic.synchronizedSum, 5);
	EXPECT_EQ(logic.synchronizedCount, 1);
}

TEST(ComponentLogicTest, RenderRunsBeginParseEndOverProcessableComponents)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(2);
	entity.addComponent<ValueComponent>(3);

	spk::ComponentRegistry components;
	for (const auto &component : entity.components())
	{
		components.add(component.get());
	}
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();
	spk::RenderUnitBuilder builder;

	logics.render(builder, components);

	EXPECT_EQ(logic.renderStartedCount, 1);
	EXPECT_EQ(logic.renderParsedCount, 2);
	EXPECT_EQ(logic.renderExecutedCount, 1);
}

TEST(ComponentLogicTest, DeactivatedLogicDoesNotRun)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(4);

	spk::ComponentRegistry components;
	components.add(entity.components().front().get());
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();
	logic.deactivate();

	spk::UpdateTick tick{};
	logics.update(tick, components);

	EXPECT_TRUE(logic.trace.empty());
}

TEST(ComponentLogicTest, UpdateSkipsNonProcessableComponents)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(9);

	spk::ComponentRegistry components;
	components.add(entity.components().front().get());

	entity.deactivate();
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	spk::UpdateTick tick{};
	logics.update(tick, components);

	// Begin/end still run, but no component is parsed.
	EXPECT_EQ(logic.trace, "SE");
	EXPECT_EQ(logic.sum, 0);
}

TEST(ComponentLogicTest, EventReachesPerComponentHook)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(1);

	spk::ComponentRegistry components;
	components.add(entity.components().front().get());
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	logics.dispatchEvent(event, components);

	EXPECT_EQ(logic.resizedCount, 1);
}

TEST(ComponentLogicTest, EventConsumedDuringStartSkipsPerComponentHook)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(1);

	spk::ComponentRegistry components;
	components.add(entity.components().front().get());
	components.refreshProcessable();

	spk::ComponentLogicRegistry logics;
	ValueLogic &logic = logics.add<ValueLogic>();
	logic.consumeResizeOnStart = true;

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	logics.dispatchEvent(event, components);

	EXPECT_EQ(logic.resizedStartedCount, 1);
	EXPECT_EQ(logic.resizedCount, 0);
	EXPECT_EQ(logic.resizedExecutedCount, 1);
	EXPECT_TRUE(event.isConsumed());
}
