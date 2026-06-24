#include <gtest/gtest.h>

#include <vector>

#include "structures/game_engine/spk_component.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/game_engine/spk_component_registry.hpp"

namespace
{
	std::vector<int> g_order;

	template <int Id>
	struct TagComponent : public spk::Component
	{
		TagComponent() = default;
	};

	template <int Id>
	class OrderLogic : public spk::ComponentLogic<TagComponent<Id>>
	{
	public:
		bool consumeOnEvent = false;

	protected:
		void _onUpdateStarted(const spk::UpdateTick &) override
		{
			g_order.push_back(Id);
		}

		void _onWindowResizedEventStarted(spk::WindowResizedEvent &p_event) override
		{
			g_order.push_back(Id);

			if (consumeOnEvent == true)
			{
				p_event.consume();
			}
		}
	};
}

TEST(ComponentLogicRegistryTest, AddReturnsSameInstanceForSameType)
{
	spk::ComponentLogicRegistry logics;

	OrderLogic<1> &first = logics.add<OrderLogic<1>>();
	OrderLogic<1> &second = logics.add<OrderLogic<1>>();

	EXPECT_EQ(&first, &second);
}

TEST(ComponentLogicRegistryTest, GetReturnsAddedLogicOrNull)
{
	spk::ComponentLogicRegistry logics;

	EXPECT_EQ(logics.get<OrderLogic<1>>(), nullptr);

	OrderLogic<1> &logic = logics.add<OrderLogic<1>>();

	EXPECT_EQ(logics.get<OrderLogic<1>>(), &logic);
	EXPECT_EQ(logics.get<OrderLogic<2>>(), nullptr);
}

TEST(ComponentLogicRegistryTest, HigherPriorityRunsFirst)
{
	g_order.clear();

	spk::ComponentLogicRegistry logics;
	logics.add<OrderLogic<1>>().setPriority(10);
	logics.add<OrderLogic<2>>().setPriority(30);
	logics.add<OrderLogic<3>>().setPriority(20);

	spk::ComponentRegistry components;
	spk::UpdateTick tick{};
	logics.update(tick, components);

	EXPECT_EQ(g_order, (std::vector<int>{2, 3, 1}));
}

TEST(ComponentLogicRegistryTest, EqualPriorityKeepsInsertionOrder)
{
	g_order.clear();

	spk::ComponentLogicRegistry logics;
	logics.add<OrderLogic<1>>();
	logics.add<OrderLogic<2>>();
	logics.add<OrderLogic<3>>();

	spk::ComponentRegistry components;
	spk::UpdateTick tick{};
	logics.update(tick, components);

	EXPECT_EQ(g_order, (std::vector<int>{1, 2, 3}));
}

TEST(ComponentLogicRegistryTest, PriorityChangeReordersBeforeNextRun)
{
	g_order.clear();

	spk::ComponentLogicRegistry logics;
	logics.add<OrderLogic<1>>();
	OrderLogic<2> &second = logics.add<OrderLogic<2>>();

	second.setPriority(100);

	spk::ComponentRegistry components;
	spk::UpdateTick tick{};
	logics.update(tick, components);

	EXPECT_EQ(g_order, (std::vector<int>{2, 1}));
}

TEST(ComponentLogicRegistryTest, DeactivatedLogicIsSkipped)
{
	g_order.clear();

	spk::ComponentLogicRegistry logics;
	logics.add<OrderLogic<1>>();
	logics.add<OrderLogic<2>>().deactivate();

	spk::ComponentRegistry components;
	spk::UpdateTick tick{};
	logics.update(tick, components);

	EXPECT_EQ(g_order, (std::vector<int>{1}));
}

TEST(ComponentLogicRegistryTest, ConsumedEventStopsLowerPriorityLogics)
{
	g_order.clear();

	spk::ComponentLogicRegistry logics;
	OrderLogic<1> &consumer = logics.add<OrderLogic<1>>();
	consumer.setPriority(100);
	consumer.consumeOnEvent = true;
	logics.add<OrderLogic<2>>().setPriority(0);

	spk::ComponentRegistry components;
	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	logics.dispatchEvent(event, components);

	EXPECT_EQ(g_order, (std::vector<int>{1}));
	EXPECT_TRUE(event.isConsumed());
}

TEST(ComponentLogicRegistryTest, ClearRemovesAllLogics)
{
	spk::ComponentLogicRegistry logics;
	logics.add<OrderLogic<1>>();

	logics.clear();

	EXPECT_EQ(logics.get<OrderLogic<1>>(), nullptr);
}
