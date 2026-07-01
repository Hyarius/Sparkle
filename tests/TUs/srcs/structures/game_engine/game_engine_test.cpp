#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/game_engine/spk_animation_logic.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_sprite_render_logic.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/event/spk_events.hpp"

namespace spk
{
	struct GameEngineTester
	{
		static void update(spk::GameEngine &p_engine, const spk::UpdateTick &p_tick) { p_engine.update(p_tick); }
		[[nodiscard]] static spk::RenderUnit buildRenderUnit(spk::GameEngine &p_engine) { return p_engine.buildRenderUnit(); }

		template <typename TEvent>
		static void dispatchEvent(spk::GameEngine &p_engine, TEvent &p_event) { p_engine.dispatchEvent(p_event); }
	};
}

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

	class PassiveValueLogic : public spk::ComponentLogic<ValueComponent>
	{
	};

	class TrackingValueLogic : public spk::ComponentLogic<ValueComponent>
	{
	public:
		int updateParses = 0;
		int updateRuns = 0;
		int renderParses = 0;
		int renderRuns = 0;
		int eventParses = 0;

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &, ValueComponent &) override { ++updateParses; }
		void _executeUpdate(const spk::UpdateTick &) override { ++updateRuns; }

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
		spk::GameEngineTester::dispatchEvent(p_engine, closeEvent);

		spk::WindowDestroyedRecord destroyedRecord;
		spk::WindowDestroyedEvent destroyedEvent(destroyedRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, destroyedEvent);

		spk::WindowMovedRecord movedRecord;
		spk::WindowMovedEvent movedEvent(movedRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, movedEvent);

		spk::WindowResizedRecord resizedRecord;
		spk::WindowResizedEvent resizedEvent(resizedRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, resizedEvent);

		spk::WindowFocusGainedRecord focusGainedRecord;
		spk::WindowFocusGainedEvent focusGainedEvent(focusGainedRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, focusGainedEvent);

		spk::WindowFocusLostRecord focusLostRecord;
		spk::WindowFocusLostEvent focusLostEvent(focusLostRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, focusLostEvent);

		spk::WindowShownRecord shownRecord;
		spk::WindowShownEvent shownEvent(shownRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, shownEvent);

		spk::WindowHiddenRecord hiddenRecord;
		spk::WindowHiddenEvent hiddenEvent(hiddenRecord);
		spk::GameEngineTester::dispatchEvent(p_engine, hiddenEvent);

		spk::MouseEnteredRecord enteredRecord;
		spk::MouseEnteredWindowEvent enteredEvent(enteredRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, enteredEvent);

		spk::MouseLeftRecord leftRecord;
		spk::MouseLeftWindowEvent leftEvent(leftRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, leftEvent);

		spk::MouseMovedRecord mouseMovedRecord;
		spk::MouseMovedEvent mouseMovedEvent(mouseMovedRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, mouseMovedEvent);

		spk::MouseWheelScrolledRecord wheelRecord;
		spk::MouseWheelScrolledEvent wheelEvent(wheelRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, wheelEvent);

		spk::MouseButtonPressedRecord pressedRecord;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, pressedEvent);

		spk::MouseButtonReleasedRecord releasedRecord;
		spk::MouseButtonReleasedEvent releasedEvent(releasedRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, releasedEvent);

		spk::MouseButtonDoubleClickedRecord doubleClickedRecord;
		spk::MouseButtonDoubleClickedEvent doubleClickedEvent(doubleClickedRecord, mouse);
		spk::GameEngineTester::dispatchEvent(p_engine, doubleClickedEvent);

		spk::KeyPressedRecord keyPressedRecord;
		spk::KeyPressedEvent keyPressedEvent(keyPressedRecord, keyboard);
		spk::GameEngineTester::dispatchEvent(p_engine, keyPressedEvent);

		spk::KeyReleasedRecord keyReleasedRecord;
		spk::KeyReleasedEvent keyReleasedEvent(keyReleasedRecord, keyboard);
		spk::GameEngineTester::dispatchEvent(p_engine, keyReleasedEvent);

		spk::TextInputRecord textRecord;
		spk::TextInputEvent textEvent(textRecord, keyboard);
		spk::GameEngineTester::dispatchEvent(p_engine, textEvent);
	}
}

TEST(GameEngineTest, UpdateDrivesLogicOverRegisteredComponents)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(7);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);

	EXPECT_EQ(logic.sum, 7);
	EXPECT_EQ(logic.updateRuns, 1);
}

TEST(GameEngineTest, IdIsNonNullAndMatchesComponentRegistry)
{
	spk::GameEngine engine;
	const spk::GameEngine &constEngine = engine;

	EXPECT_FALSE(engine.id().isNull());
	EXPECT_EQ(constEngine.id(), engine.componentRegistry().engineId());
}

TEST(GameEngineTest, NullEntityOperationsAreNoOps)
{
	spk::GameEngine engine;

	EXPECT_NO_THROW(engine.addEntity(nullptr));
	EXPECT_NO_THROW(engine.removeEntity(nullptr));
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

TEST(GameEngineTest, RemoveEntityRemovesItsComponentsFromProcessing)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(5);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);
	ASSERT_EQ(logic.sum, 5);

	engine.removeEntity(&entity);
	spk::GameEngineTester::update(engine, tick);

	EXPECT_EQ(logic.sum, 0);
}

TEST(GameEngineTest, RemoveEntityIgnoresEntityNotRegisteredHere)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity registered;
	engine.addEntity(&registered);
	registered.addComponent<ValueComponent>(4);

	spk::Entity stray;
	stray.addComponent<ValueComponent>(99);

	engine.removeEntity(&stray);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);

	EXPECT_EQ(logic.sum, 4);
}

TEST(GameEngineTest, RenderDrivesRenderHook)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	(void)spk::GameEngineTester::buildRenderUnit(engine);

	EXPECT_EQ(logic.renderRuns, 1);
}

TEST(GameEngineTest, DispatchEventReachesLogic)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	spk::GameEngineTester::dispatchEvent(engine, event);

	EXPECT_EQ(logic.resizedCount, 1);
}

TEST(GameEngineTest, EntityActivationChangesInvalidateProcessableComponents)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	spk::WindowResizedRecord record;

	spk::WindowResizedEvent firstEvent(record);
	spk::GameEngineTester::dispatchEvent(engine, firstEvent);
	ASSERT_EQ(logic.resizedCount, 1);

	entity.deactivate();

	spk::WindowResizedEvent deactivatedEvent(record);
	spk::GameEngineTester::dispatchEvent(engine, deactivatedEvent);
	EXPECT_EQ(logic.resizedCount, 1);

	entity.activate();

	spk::WindowResizedEvent reactivatedEvent(record);
	spk::GameEngineTester::dispatchEvent(engine, reactivatedEvent);
	EXPECT_EQ(logic.resizedCount, 2);
}

TEST(GameEngineTest, DeactivatedEngineSkipsUpdateRenderAndDispatch)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(3);

	engine.deactivate();

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);
	(void)spk::GameEngineTester::buildRenderUnit(engine);

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	spk::GameEngineTester::dispatchEvent(engine, event);

	EXPECT_EQ(logic.updateRuns, 0);
	EXPECT_EQ(logic.renderRuns, 0);
	EXPECT_EQ(logic.resizedCount, 0);
}

TEST(GameEngineTest, ComponentAddedAfterEntityIsPickedUpOnNextUpdate)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity entity;
	engine.addEntity(&entity);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);
	ASSERT_EQ(logic.sum, 0);

	entity.addComponent<ValueComponent>(4);
	spk::GameEngineTester::update(engine, tick);

	EXPECT_EQ(logic.sum, 4);
}

TEST(GameEngineTest, EveryPhaseAndEventTypeDrivesLogicHooks)
{
	spk::GameEngine engine;
	engine.add<PassiveValueLogic>();
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);
	(void)spk::GameEngineTester::buildRenderUnit(engine);

	dispatchEveryEventType(engine);

	EXPECT_EQ(tracker.updateRuns, 1);
	EXPECT_EQ(tracker.updateParses, 1);
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

	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);
	entity.addComponent<ValueComponent>(2);

	spk::WindowResizedRecord record;
	spk::WindowResizedEvent event(record);
	spk::GameEngineTester::dispatchEvent(engine, event);

	EXPECT_EQ(consumer.parses, 1);
	EXPECT_EQ(tracker.eventParses, 0);
}

TEST(GameEngineTest, DeactivatedLogicIsSkippedInEveryPhase)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	tracker.deactivate();

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);
	(void)spk::GameEngineTester::buildRenderUnit(engine);

	spk::WindowShownRecord record;
	spk::WindowShownEvent event(record);
	spk::GameEngineTester::dispatchEvent(engine, event);

	EXPECT_EQ(tracker.updateRuns, 0);
	EXPECT_EQ(tracker.renderRuns, 0);
	EXPECT_EQ(tracker.eventParses, 0);
}

TEST(GameEngineTest, DeactivatedEngineSkipsRenderAndDispatch)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();
	spk::Entity entity;
	engine.addEntity(&entity);
	entity.addComponent<ValueComponent>(1);

	engine.deactivate();

	(void)spk::GameEngineTester::buildRenderUnit(engine);

	spk::WindowShownRecord record;
	spk::WindowShownEvent event(record);
	spk::GameEngineTester::dispatchEvent(engine, event);

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

TEST(GameEngineTest, AddingEntityWithChildrenCascadesRegistration)
{
	spk::GameEngine engine;
	TrackingValueLogic &tracker = engine.add<TrackingValueLogic>();

	ChildOwningEntity root;
	engine.addEntity(&root);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);

	EXPECT_EQ(tracker.updateParses, 1);
}

TEST(GameEngineTest, EntityOutlivingEngineIsSafelyDetached)
{
	spk::Entity entity;
	entity.addComponent<ValueComponent>(1);

	{
		spk::GameEngine engine;
		engine.add<SumLogic>();
		engine.addEntity(&entity);

		spk::UpdateTick tick{};
		spk::GameEngineTester::update(engine, tick);
	}

	EXPECT_NO_THROW(entity.addComponent<ValueComponent>(2));
	EXPECT_EQ(entity.components().size(), 2u);
}

TEST(GameEngineTest, EnginesProcessOnlyTheirOwnEntities)
{
	spk::GameEngine engineA;
	spk::GameEngine engineB;
	SumLogic &logicA = engineA.add<SumLogic>();
	SumLogic &logicB = engineB.add<SumLogic>();

	spk::Entity entityA;
	engineA.addEntity(&entityA);
	entityA.addComponent<ValueComponent>(10);

	spk::Entity entityB;
	engineB.addEntity(&entityB);
	entityB.addComponent<ValueComponent>(20);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engineA, tick);
	spk::GameEngineTester::update(engineB, tick);

	EXPECT_EQ(logicA.sum, 10);
	EXPECT_EQ(logicB.sum, 20);
}

TEST(GameEngineTest, ReparentingMigratesComponentsBetweenEngines)
{
	spk::GameEngine engineA;
	spk::GameEngine engineB;
	SumLogic &logicA = engineA.add<SumLogic>();
	SumLogic &logicB = engineB.add<SumLogic>();

	spk::Entity rootB;
	engineB.addEntity(&rootB);

	spk::Entity child;
	engineA.addEntity(&child);
	child.addComponent<ValueComponent>(5);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engineA, tick);
	spk::GameEngineTester::update(engineB, tick);
	ASSERT_EQ(logicA.sum, 5);
	ASSERT_EQ(logicB.sum, 0);

	child.setParent(&rootB);

	spk::GameEngineTester::update(engineA, tick);
	spk::GameEngineTester::update(engineB, tick);

	EXPECT_EQ(logicA.sum, 0);
	EXPECT_EQ(logicB.sum, 5);
}

TEST(GameEngineTest, DeactivatingParentStopsChildComponentsFromBeingProcessed)
{
	spk::GameEngine engine;
	SumLogic &logic = engine.add<SumLogic>();

	spk::Entity parent;
	engine.addEntity(&parent);
	spk::Entity child(&parent);
	child.addComponent<ValueComponent>(8);

	spk::UpdateTick tick{};
	spk::GameEngineTester::update(engine, tick);
	ASSERT_EQ(logic.sum, 8);

	parent.deactivate();
	spk::GameEngineTester::update(engine, tick);
	EXPECT_EQ(logic.sum, 0);

	parent.activate();
	spk::GameEngineTester::update(engine, tick);
	EXPECT_EQ(logic.sum, 8);
}

TEST(AnimationLogicTest, ControllerWithoutRendererIsIgnored)
{
	spk::GameEngine engine;
	engine.add<spk::AnimationLogic>();
	spk::Entity2D entity;
	engine.addEntity(&entity);
	spk::AnimationController2D &controller = entity.addComponent<spk::AnimationController2D>();
	spk::Animation2D animation;
	animation.frames = {{0u, 0u}};
	controller.addAnimation(L"idle", animation);
	controller.play(L"idle");

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(0.5L, spk::TimeUnit::Second);
	spk::GameEngineTester::update(engine, tick);

	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 0.0);
}

TEST(AnimationLogicTest, RendererWithoutSpriteSheetIsIgnored)
{
	spk::GameEngine engine;
	engine.add<spk::AnimationLogic>();
	spk::Entity2D entity;
	engine.addEntity(&entity);
	spk::AnimationController2D &controller = entity.addComponent<spk::AnimationController2D>();
	entity.addComponent<spk::SpriteRenderer2D>();
	spk::Animation2D animation;
	animation.frames = {{0u, 0u}};
	controller.addAnimation(L"idle", animation);
	controller.play(L"idle");

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(0.5L, spk::TimeUnit::Second);
	spk::GameEngineTester::update(engine, tick);

	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 0.0);
}

TEST(AnimationLogicTest, EmptyAnimationIsIgnored)
{
	spk::GameEngine engine;
	engine.add<spk::AnimationLogic>();
	spk::Entity2D entity;
	engine.addEntity(&entity);
	spk::AnimationController2D &controller = entity.addComponent<spk::AnimationController2D>();
	spk::SpriteRenderer2D &renderer = entity.addComponent<spk::SpriteRenderer2D>();
	spk::SpriteSheet emptySheet;
	renderer.setSpriteSheet(&emptySheet);
	controller.addAnimation(L"empty", spk::Animation2D{});
	controller.play(L"empty");

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(0.5L, spk::TimeUnit::Second);
	spk::GameEngineTester::update(engine, tick);

	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 0.0);
}

TEST(AnimationLogicTest, LoopingAnimationAdvancesBeforeInvalidSheetLookup)
{
	spk::GameEngine engine;
	engine.add<spk::AnimationLogic>();
	spk::Entity2D entity;
	engine.addEntity(&entity);
	spk::AnimationController2D &controller = entity.addComponent<spk::AnimationController2D>();
	spk::SpriteRenderer2D &renderer = entity.addComponent<spk::SpriteRenderer2D>();
	spk::SpriteSheet emptySheet;
	renderer.setSpriteSheet(&emptySheet);
	spk::Animation2D animation;
	animation.frames = {{0u, 0u}, {1u, 0u}};
	animation.frameDuration = spk::Duration(0.25L, spk::TimeUnit::Second);
	animation.loop = true;
	controller.addAnimation(L"walk", animation);
	controller.play(L"walk");

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(0.75L, spk::TimeUnit::Second);
	EXPECT_THROW(spk::GameEngineTester::update(engine, tick), std::out_of_range);

	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 0.75);
}

TEST(AnimationLogicTest, NonLoopingAnimationClampsBeforeInvalidSheetLookup)
{
	spk::GameEngine engine;
	engine.add<spk::AnimationLogic>();
	spk::Entity2D entity;
	engine.addEntity(&entity);
	spk::AnimationController2D &controller = entity.addComponent<spk::AnimationController2D>();
	spk::SpriteRenderer2D &renderer = entity.addComponent<spk::SpriteRenderer2D>();
	spk::SpriteSheet emptySheet;
	renderer.setSpriteSheet(&emptySheet);
	spk::Animation2D animation;
	animation.frames = {{0u, 0u}, {1u, 0u}};
	animation.frameDuration = spk::Duration();
	animation.loop = false;
	controller.addAnimation(L"once", animation);
	controller.play(L"once");

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(1.0L, spk::TimeUnit::Second);
	EXPECT_THROW(spk::GameEngineTester::update(engine, tick), std::out_of_range);

	EXPECT_DOUBLE_EQ(controller.elapsedSeconds(), 1.0);
}

TEST(SpriteRenderLogicTest, RenderWithoutMainCameraProducesNoSprites)
{
	spk::GameEngine engine;
	engine.add<spk::SpriteRenderLogic>();
	spk::Entity2D entity;
	engine.addEntity(&entity);
	entity.addComponent<spk::SpriteRenderer2D>();

	(void)spk::GameEngineTester::buildRenderUnit(engine);

	EXPECT_EQ(spk::SpriteRenderLogic::lastSpriteCount(), 0u);
	EXPECT_EQ(spk::SpriteRenderLogic::lastPolygonCount(), 0u);
}

TEST(SpriteRenderLogicTest, ZeroSizedMainCameraViewportSkipsSprite)
{
	spk::GameEngine engine;
	engine.add<spk::SpriteRenderLogic>();
	spk::Entity2D cameraEntity;
	spk::Camera2D &camera = cameraEntity.addComponent<spk::Camera2D>();
	camera.makeMain();
	spk::Entity2D spriteEntity;
	engine.addEntity(&spriteEntity);
	spk::SpriteRenderer2D &renderer = spriteEntity.addComponent<spk::SpriteRenderer2D>();
	spk::SpriteSheet sheet;
	spk::TextureMesh2D mesh;
	renderer.setSpriteSheet(&sheet);
	renderer.setMesh(&mesh);

	(void)spk::GameEngineTester::buildRenderUnit(engine);

	EXPECT_EQ(spk::SpriteRenderLogic::lastSpriteCount(), 0u);
	EXPECT_EQ(spk::SpriteRenderLogic::lastPolygonCount(), 0u);
}

TEST(SpriteRenderLogicTest, ValidSpritesAreBatchedAndCounted)
{
	spk::GameEngine engine;
	engine.add<spk::SpriteRenderLogic>();
	spk::Entity2D cameraEntity;
	spk::Camera2D &camera = cameraEntity.addComponent<spk::Camera2D>();
	camera.setViewport(spk::Rect2D(0, 0, 640, 480));
	camera.makeMain();
	spk::SpriteSheet sheet;
	spk::TextureMesh2D mesh;

	spk::Entity2D firstEntity;
	engine.addEntity(&firstEntity);
	spk::SpriteRenderer2D &first = firstEntity.addComponent<spk::SpriteRenderer2D>();
	first.setSpriteSheet(&sheet);
	first.setMesh(&mesh);

	spk::Entity2D secondEntity;
	engine.addEntity(&secondEntity);
	spk::SpriteRenderer2D &second = secondEntity.addComponent<spk::SpriteRenderer2D>();
	second.setSpriteSheet(&sheet);
	second.setMesh(&mesh);

	spk::RenderUnit unit = spk::GameEngineTester::buildRenderUnit(engine);

	EXPECT_EQ(spk::SpriteRenderLogic::lastSpriteCount(), 2u);
	EXPECT_EQ(spk::SpriteRenderLogic::lastPolygonCount(), 4u);
	EXPECT_FALSE(unit.commands().empty());
}
