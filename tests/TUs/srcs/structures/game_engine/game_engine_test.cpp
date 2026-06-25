#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/event/spk_events.hpp"

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
		int synchronizeRuns = 0;
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

		void _executeSynchronization() override
		{
			++synchronizeRuns;
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

	class PassiveValueLogic : public spk::ComponentLogic<ValueComponent>
	{
	};

	class TrackingValueLogic : public spk::ComponentLogic<ValueComponent>
	{
	public:
		int updateParses = 0;
		int updateRuns = 0;
		int syncParses = 0;
		int syncRuns = 0;
		int renderParses = 0;
		int renderRuns = 0;
		int eventParses = 0;

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &, ValueComponent &) override { ++updateParses; }
		void _executeUpdate(const spk::UpdateTick &) override { ++updateRuns; }

		void _parseComponentForSynchronization(ValueComponent &) override { ++syncParses; }
		void _executeSynchronization() override { ++syncRuns; }

		void _parseComponentForRender(ValueComponent &) override { ++renderParses; }
		void _executeRender(spk::RenderUnitBuilder &) override { ++renderRuns; }

		void _parseComponentForWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowDestroyedEvent(spk::WindowDestroyedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowMovedEvent(spk::WindowMovedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowFocusGainedEvent(spk::WindowFocusGainedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowFocusLostEvent(spk::WindowFocusLostEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowShownEvent(spk::WindowShownEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForWindowHiddenEvent(spk::WindowHiddenEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseEnteredEvent(spk::MouseEnteredWindowEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseLeftEvent(spk::MouseLeftWindowEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForKeyPressedEvent(spk::KeyPressedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForKeyReleasedEvent(spk::KeyReleasedEvent &, ValueComponent &) override { ++eventParses; }
		void _parseComponentForTextInputEvent(spk::TextInputEvent &, ValueComponent &) override { ++eventParses; }
	};

	class ResizeConsumingValueLogic : public spk::ComponentLogic<ValueComponent>
	{
	public:
		int parses = 0;

	protected:
		void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &p_event, ValueComponent &) override
		{
			++parses;
			p_event.consume();
		}
	};

	class ChildOwningEntity : public spk::Entity
	{
	public:
		std::unique_ptr<spk::Entity> child;

		ChildOwningEntity()
		{
			child = std::make_unique<spk::Entity>(this);
			child->addComponent<ValueComponent>(7);
		}
	};

	void dispatchEveryEventType(spk::GameEngine &p_engine)
	{
		spk::Mouse mouse;
		spk::Keyboard keyboard;

		spk::WindowCloseRequestedRecord closeRecord;
		spk::WindowCloseRequestedEvent closeEvent(closeRecord);
		p_engine.dispatchEvent(closeEvent);

		spk::WindowDestroyedRecord destroyedRecord;
		spk::WindowDestroyedEvent destroyedEvent(destroyedRecord);
		p_engine.dispatchEvent(destroyedEvent);

		spk::WindowMovedRecord movedRecord;
		spk::WindowMovedEvent movedEvent(movedRecord);
		p_engine.dispatchEvent(movedEvent);

		spk::WindowResizedRecord resizedRecord;
		spk::WindowResizedEvent resizedEvent(resizedRecord);
		p_engine.dispatchEvent(resizedEvent);

		spk::WindowFocusGainedRecord focusGainedRecord;
		spk::WindowFocusGainedEvent focusGainedEvent(focusGainedRecord);
		p_engine.dispatchEvent(focusGainedEvent);

		spk::WindowFocusLostRecord focusLostRecord;
		spk::WindowFocusLostEvent focusLostEvent(focusLostRecord);
		p_engine.dispatchEvent(focusLostEvent);

		spk::WindowShownRecord shownRecord;
		spk::WindowShownEvent shownEvent(shownRecord);
		p_engine.dispatchEvent(shownEvent);

		spk::WindowHiddenRecord hiddenRecord;
		spk::WindowHiddenEvent hiddenEvent(hiddenRecord);
		p_engine.dispatchEvent(hiddenEvent);

		spk::MouseEnteredRecord enteredRecord;
		spk::MouseEnteredWindowEvent enteredEvent(enteredRecord, mouse);
		p_engine.dispatchEvent(enteredEvent);

		spk::MouseLeftRecord leftRecord;
		spk::MouseLeftWindowEvent leftEvent(leftRecord, mouse);
		p_engine.dispatchEvent(leftEvent);

		spk::MouseMovedRecord mouseMovedRecord;
		spk::MouseMovedEvent mouseMovedEvent(mouseMovedRecord, mouse);
		p_engine.dispatchEvent(mouseMovedEvent);

		spk::MouseWheelScrolledRecord wheelRecord;
		spk::MouseWheelScrolledEvent wheelEvent(wheelRecord, mouse);
		p_engine.dispatchEvent(wheelEvent);

		spk::MouseButtonPressedRecord pressedRecord;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		p_engine.dispatchEvent(pressedEvent);

		spk::MouseButtonReleasedRecord releasedRecord;
		spk::MouseButtonReleasedEvent releasedEvent(releasedRecord, mouse);
		p_engine.dispatchEvent(releasedEvent);

		spk::MouseButtonDoubleClickedRecord doubleClickedRecord;
		spk::MouseButtonDoubleClickedEvent doubleClickedEvent(doubleClickedRecord, mouse);
		p_engine.dispatchEvent(doubleClickedEvent);

		spk::KeyPressedRecord keyPressedRecord;
		spk::KeyPressedEvent keyPressedEvent(keyPressedRecord, keyboard);
		p_engine.dispatchEvent(keyPressedEvent);

		spk::KeyReleasedRecord keyReleasedRecord;
		spk::KeyReleasedEvent keyReleasedEvent(keyReleasedRecord, keyboard);
		p_engine.dispatchEvent(keyReleasedEvent);

		spk::TextInputRecord textRecord;
		spk::TextInputEvent textEvent(textRecord, keyboard);
		p_engine.dispatchEvent(textEvent);
	}
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

TEST(GameEngineTest, ConstAccessorsReturnEngineRegistriesAndLogic)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	const spk::GameEngine &constEngine = engine;

	EXPECT_EQ(static_cast<const spk::ComponentRegistry *>(&engine.componentRegistry()), &constEngine.componentRegistry());
	EXPECT_EQ(static_cast<const spk::ComponentLogicRegistry *>(&engine.logicRegistry()), &constEngine.logicRegistry());
	EXPECT_EQ(constEngine.logic<SumLogic>(), static_cast<const SumLogic *>(&logic));
}

TEST(GameEngineTest, RequireLogicReturnsExistingLogicAndThrowsWhenMissing)
{
	spk::GameEngine engine;

	EXPECT_THROW((void)engine.requireLogic<SumLogic>(), std::runtime_error);

	SumLogic &logic = engine.add<SumLogic>();

	EXPECT_EQ(&engine.requireLogic<SumLogic>(), &logic);
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

TEST(GameEngineTest, EntityActivationChangesInvalidateProcessableComponents)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	spk::Entity &entity = engine.addEntity<spk::Entity>();
	entity.addComponent<ValueComponent>(1);

	spk::WindowResizedRecord record;

	spk::WindowResizedEvent firstEvent(record);
	engine.dispatchEvent(firstEvent);
	ASSERT_EQ(logic.resizedCount, 1);

	entity.deactivate();

	spk::WindowResizedEvent deactivatedEvent(record);
	engine.dispatchEvent(deactivatedEvent);
	EXPECT_EQ(logic.resizedCount, 1);

	entity.activate();

	spk::WindowResizedEvent reactivatedEvent(record);
	engine.dispatchEvent(reactivatedEvent);
	EXPECT_EQ(logic.resizedCount, 2);
}

TEST(GameEngineTest, DeactivatedEngineSkipsUpdateSynchronizeRenderAndDispatch)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(3);

	engine.deactivate();

	spk::UpdateTick tick{};
	engine.update(tick);
	engine.synchronize();

	spk::RenderUnitBuilder builder;
	engine.render(builder);

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	engine.dispatchEvent(event);

	EXPECT_EQ(logic.updateRuns, 0);
	EXPECT_EQ(logic.synchronizeRuns, 0);
	EXPECT_EQ(logic.renderRuns, 0);
	EXPECT_EQ(logic.resizedCount, 0);
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

TEST(GameEngineTest, EveryPhaseAndEventTypeDrivesLogicHooks)
{
	spk::GameEngine engine;
	engine.add<PassiveValueLogic>();
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(1);

	spk::UpdateTick tick{};
	engine.update(tick);
	engine.synchronize();

	spk::RenderUnitBuilder builder;
	engine.render(builder);

	dispatchEveryEventType(engine);

	EXPECT_EQ(tracker.updateRuns, 1);
	EXPECT_EQ(tracker.updateParses, 1);
	EXPECT_EQ(tracker.syncRuns, 1);
	EXPECT_EQ(tracker.syncParses, 1);
	EXPECT_EQ(tracker.renderRuns, 1);
	EXPECT_EQ(tracker.renderParses, 1);
	EXPECT_EQ(tracker.eventParses, 18);
}

TEST(GameEngineTest, ConsumingLogicStopsPerComponentLoopAndLaterLogics)
{
	spk::GameEngine engine;
	ResizeConsumingValueLogic &consumer = engine.add<ResizeConsumingValueLogic>();
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	consumer.setPriority(10);

	spk::Entity &entity = engine.addEntity<spk::Entity>();
	entity.addComponent<ValueComponent>(1);
	entity.addComponent<ValueComponent>(2);

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	engine.dispatchEvent(event);

	EXPECT_EQ(consumer.parses, 1);
	EXPECT_EQ(tracker.eventParses, 0);
}

TEST(GameEngineTest, DeactivatedLogicIsSkippedInEveryPhase)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(1);

	tracker.deactivate();

	spk::UpdateTick tick{};
	engine.update(tick);
	engine.synchronize();

	spk::RenderUnitBuilder builder;
	engine.render(builder);

	spk::WindowShownRecord record;
	spk::WindowShownEvent event(record);
	engine.dispatchEvent(event);

	EXPECT_EQ(tracker.updateRuns, 0);
	EXPECT_EQ(tracker.syncRuns, 0);
	EXPECT_EQ(tracker.renderRuns, 0);
	EXPECT_EQ(tracker.eventParses, 0);
}

TEST(GameEngineTest, DeactivatedEngineSkipsSynchronizeRenderAndDispatch)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	engine.addEntity<spk::Entity>().addComponent<ValueComponent>(1);

	engine.deactivate();

	engine.synchronize();

	spk::RenderUnitBuilder builder;
	engine.render(builder);

	spk::WindowShownRecord record;
	spk::WindowShownEvent event(record);
	engine.dispatchEvent(event);

	EXPECT_EQ(tracker.syncRuns, 0);
	EXPECT_EQ(tracker.renderRuns, 0);
	EXPECT_EQ(tracker.eventParses, 0);
}

TEST(GameEngineTest, LogicLookupHelpersCoverFoundAndMissingPaths)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();

	EXPECT_EQ(engine.logic<TrackingValueLogic>(), &tracker);
	EXPECT_EQ(engine.logic<PassiveValueLogic>(), nullptr);
	EXPECT_EQ(&engine.requireLogic<TrackingValueLogic>(), &tracker);
	EXPECT_THROW((void)engine.requireLogic<PassiveValueLogic>(), std::runtime_error);
	EXPECT_EQ(&engine.add<TrackingValueLogic>(), &tracker);
}

TEST(GameEngineTest, DestroyEntityIgnoresEntityOwnedElsewhere)
{
	spk::GameEngine engine;
	spk::Entity stray;

	engine.destroyEntity(stray);

	EXPECT_TRUE(engine.entities().empty());
}

TEST(GameEngineTest, AddingEntityWithChildrenCascadesRegistration)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();

	engine.addEntity<ChildOwningEntity>();

	spk::UpdateTick tick{};
	engine.update(tick);

	EXPECT_EQ(tracker.updateParses, 1);
}
