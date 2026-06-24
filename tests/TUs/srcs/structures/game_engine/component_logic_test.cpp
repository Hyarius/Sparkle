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
		int resizedCount = 0;

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

		void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &, ValueComponent &) override
		{
			++resizedCount;
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
