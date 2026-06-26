#include <gtest/gtest.h>

#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/event/spk_events.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"

namespace
{
	class ExposedGameEngineWidget : public spk::GameEngineWidget
	{
	public:
		using spk::GameEngineWidget::GameEngineWidget;

		using spk::GameEngineWidget::_buildRenderUnit;
		using spk::GameEngineWidget::_onGeometryChange;
		using spk::GameEngineWidget::_onUpdate;

		using spk::GameEngineWidget::_onWindowCloseRequestedEvent;
		using spk::GameEngineWidget::_onWindowDestroyedEvent;
		using spk::GameEngineWidget::_onWindowFocusGainedEvent;
		using spk::GameEngineWidget::_onWindowFocusLostEvent;
		using spk::GameEngineWidget::_onWindowHiddenEvent;
		using spk::GameEngineWidget::_onWindowMovedEvent;
		using spk::GameEngineWidget::_onWindowResizedEvent;
		using spk::GameEngineWidget::_onWindowShownEvent;

		using spk::GameEngineWidget::_onMouseButtonDoubleClickedEvent;
		using spk::GameEngineWidget::_onMouseButtonPressedEvent;
		using spk::GameEngineWidget::_onMouseButtonReleasedEvent;
		using spk::GameEngineWidget::_onMouseEnteredEvent;
		using spk::GameEngineWidget::_onMouseLeftEvent;
		using spk::GameEngineWidget::_onMouseMovedEvent;
		using spk::GameEngineWidget::_onMouseWheelScrolledEvent;

		using spk::GameEngineWidget::_onKeyPressedEvent;
		using spk::GameEngineWidget::_onKeyReleasedEvent;
		using spk::GameEngineWidget::_onTextInputEvent;
	};

	void dispatchEveryEventTo(ExposedGameEngineWidget &p_widget)
	{
		spk::Mouse mouse;
		spk::Keyboard keyboard;

		spk::WindowCloseRequestedRecord closeRecord;
		spk::WindowCloseRequestedEvent closeEvent(closeRecord);
		p_widget._onWindowCloseRequestedEvent(closeEvent);

		spk::WindowDestroyedRecord destroyedRecord;
		spk::WindowDestroyedEvent destroyedEvent(destroyedRecord);
		p_widget._onWindowDestroyedEvent(destroyedEvent);

		spk::WindowMovedRecord movedRecord;
		spk::WindowMovedEvent movedEvent(movedRecord);
		p_widget._onWindowMovedEvent(movedEvent);

		spk::WindowResizedRecord resizedRecord;
		spk::WindowResizedEvent resizedEvent(resizedRecord);
		p_widget._onWindowResizedEvent(resizedEvent);

		spk::WindowFocusGainedRecord focusGainedRecord;
		spk::WindowFocusGainedEvent focusGainedEvent(focusGainedRecord);
		p_widget._onWindowFocusGainedEvent(focusGainedEvent);

		spk::WindowFocusLostRecord focusLostRecord;
		spk::WindowFocusLostEvent focusLostEvent(focusLostRecord);
		p_widget._onWindowFocusLostEvent(focusLostEvent);

		spk::WindowShownRecord shownRecord;
		spk::WindowShownEvent shownEvent(shownRecord);
		p_widget._onWindowShownEvent(shownEvent);

		spk::WindowHiddenRecord hiddenRecord;
		spk::WindowHiddenEvent hiddenEvent(hiddenRecord);
		p_widget._onWindowHiddenEvent(hiddenEvent);

		spk::MouseEnteredRecord enteredRecord;
		spk::MouseEnteredWindowEvent enteredEvent(enteredRecord, mouse);
		p_widget._onMouseEnteredEvent(enteredEvent);

		spk::MouseLeftRecord leftRecord;
		spk::MouseLeftWindowEvent leftEvent(leftRecord, mouse);
		p_widget._onMouseLeftEvent(leftEvent);

		spk::MouseMovedRecord mouseMovedRecord;
		spk::MouseMovedEvent mouseMovedEvent(mouseMovedRecord, mouse);
		p_widget._onMouseMovedEvent(mouseMovedEvent);

		spk::MouseWheelScrolledRecord wheelRecord;
		spk::MouseWheelScrolledEvent wheelEvent(wheelRecord, mouse);
		p_widget._onMouseWheelScrolledEvent(wheelEvent);

		spk::MouseButtonPressedRecord pressedRecord;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		p_widget._onMouseButtonPressedEvent(pressedEvent);

		spk::MouseButtonReleasedRecord releasedRecord;
		spk::MouseButtonReleasedEvent releasedEvent(releasedRecord, mouse);
		p_widget._onMouseButtonReleasedEvent(releasedEvent);

		spk::MouseButtonDoubleClickedRecord doubleClickedRecord;
		spk::MouseButtonDoubleClickedEvent doubleClickedEvent(doubleClickedRecord, mouse);
		p_widget._onMouseButtonDoubleClickedEvent(doubleClickedEvent);

		spk::KeyPressedRecord keyPressedRecord;
		spk::KeyPressedEvent keyPressedEvent(keyPressedRecord, keyboard);
		p_widget._onKeyPressedEvent(keyPressedEvent);

		spk::KeyReleasedRecord keyReleasedRecord;
		spk::KeyReleasedEvent keyReleasedEvent(keyReleasedRecord, keyboard);
		p_widget._onKeyReleasedEvent(keyReleasedEvent);

		spk::TextInputRecord textRecord;
		spk::TextInputEvent textEvent(textRecord, keyboard);
		p_widget._onTextInputEvent(textEvent);
	}
}

TEST(GameEngineWidgetTest, OwnsAnEngineByDefault)
{
	spk::GameEngineWidget widget("Game");

	EXPECT_NO_THROW((void)widget.gameEngine());
}

TEST(GameEngineWidgetTest, GameEngineAccessorsReturnTheOwnedEngine)
{
	spk::GameEngineWidget widget("Game");
	const spk::GameEngineWidget &constWidget = widget;

	EXPECT_EQ(&constWidget.gameEngine(), &widget.gameEngine());
}

TEST(GameEngineWidgetTest, BuildsAnEmptyRenderUnitWithoutCrashing)
{
	spk::GameEngineWidget widget("Game");
	widget.setGeometry(spk::Rect2D(0, 0, 32, 32));

	auto renderUnit = widget.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
}

TEST(GameEngineWidgetTest, UpdateRunsOwnedEngine)
{
	ExposedGameEngineWidget widget("Game");
	spk::UpdateTick tick{};

	EXPECT_NO_THROW(widget._onUpdate(tick));
}

TEST(GameEngineWidgetTest, BuildRenderUnitRendersOwnedEngine)
{
	ExposedGameEngineWidget widget("Game");

	const spk::RenderUnit unit = widget._buildRenderUnit();
	EXPECT_EQ(unit.size(), 0u);
}

TEST(GameEngineWidgetTest, GeometryChangeInvalidatesRenderUnit)
{
	ExposedGameEngineWidget widget("Game");

	EXPECT_NO_THROW(widget._onGeometryChange());
}

TEST(GameEngineWidgetTest, ForwardsEveryEventTypeToAttachedEngine)
{
	ExposedGameEngineWidget widget("Game");

	EXPECT_NO_THROW(dispatchEveryEventTo(widget));
}
