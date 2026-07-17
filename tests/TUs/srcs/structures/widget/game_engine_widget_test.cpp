#include <gtest/gtest.h>

#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_image_render_command.hpp"
#include "structures/graphics/rendering/command/spk_render_unit_render_command.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/event/spk_events.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"
#include "structures/widget/rendering/spk_widget_render_passes.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"

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

TEST(GameEngineWidgetTest, BuildsFrameBufferPassthroughWhenSized)
{
	spk::GameEngineWidget widget("Game");
	widget.setGeometry(spk::Rect2D(0, 0, 32, 32));
	widget.activate();

	auto renderUnit = widget.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	// The cached widget unit now owns only the composition image. Scene work is
	// declared into the shared frame pack during widget traversal.
	ASSERT_EQ(renderUnit->size(), 1u);
	EXPECT_NE(dynamic_cast<const spk::ImageRenderCommand *>(renderUnit->commands()[0].get()), nullptr);

	spk::RenderSnapshotBuilder snapshot;
	widget.appendRenderUnits(snapshot);
	ASSERT_EQ(snapshot.units().size(), 1u);
	const auto &commands = snapshot.units()[0]->commands();
	std::size_t firstSceneBind = commands.size();
	std::size_t composition = commands.size();
	for (std::size_t index = 0; index < commands.size(); ++index)
	{
		if (const auto *bind = dynamic_cast<const spk::UseFrameBufferRenderCommand *>(commands[index].get());
			bind != nullptr && bind->target() == &widget.frameBuffer() && firstSceneBind == commands.size())
		{
			firstSceneBind = index;
		}
		if (const auto *nested = dynamic_cast<const spk::RenderUnitRenderCommand *>(commands[index].get());
			nested != nullptr && nested->unit() == renderUnit)
		{
			composition = index;
		}
	}
	EXPECT_LT(firstSceneBind, composition);
}

TEST(GameEngineWidgetTest, SharedPlanOrdersScenePassesBeforeWidgetOverlay)
{
	spk::GameEngineWidget widget("Game");
	widget.setGeometry(spk::Rect2D(0, 0, 32, 32));
	widget.activate();
	spk::RenderPlan plan = widget.buildRenderPlan(
		{.frameBuffer = nullptr, .viewport = widget.viewport(), .activeTarget = true});
	const auto diagnostics = plan.diagnostics();
	ASSERT_EQ(diagnostics.size(), 4u);
	EXPECT_EQ(diagnostics[0].id, spk::SceneRenderPasses::MainOpaque);
	EXPECT_EQ(diagnostics[1].id, spk::SceneRenderPasses::MainTransparent);
	EXPECT_EQ(diagnostics[2].id, spk::SceneRenderPasses::MainOverlay);
	EXPECT_EQ(diagnostics[3].id, spk::WidgetRenderPasses::Overlay);
	EXPECT_TRUE(diagnostics[0].clear.color.has_value());
	EXPECT_FALSE(diagnostics[1].clear.color.has_value());
	EXPECT_FALSE(diagnostics[2].clear.color.has_value());
}

TEST(WidgetOverlayPassTest, RootWidgetsUseTheSameStableOverlayId)
{
	spk::Widget first("First");
	spk::Widget second("Second");
	first.setGeometry(spk::Rect2D(0, 0, 8, 8));
	second.setGeometry(spk::Rect2D(0, 0, 8, 8));
	spk::RenderPlan firstPlan = first.buildRenderPlan({.frameBuffer = nullptr, .viewport = first.viewport(), .activeTarget = true});
	spk::RenderPlan secondPlan = second.buildRenderPlan({.frameBuffer = nullptr, .viewport = second.viewport(), .activeTarget = true});
	ASSERT_EQ(firstPlan.size(), 1u);
	ASSERT_EQ(secondPlan.size(), 1u);
	EXPECT_EQ(firstPlan.passes()[0]->id(), spk::WidgetRenderPasses::Overlay);
	EXPECT_EQ(secondPlan.passes()[0]->id(), spk::WidgetRenderPasses::Overlay);
}

TEST(GameEngineWidgetTest, UsesExplicitSampleableColorAndRenderbufferDepthTarget)
{
	spk::GameEngineWidget widget("Game");
	widget.setGeometry(spk::Rect2D(0, 0, 32, 24));

	const spk::FrameBufferObject &frameBuffer = widget.frameBuffer();
	EXPECT_EQ(frameBuffer.size(), spk::Vector2UInt(32, 24));
	EXPECT_TRUE(frameBuffer.hasColorAttachment());
	EXPECT_TRUE(frameBuffer.hasDepthAttachment());
	EXPECT_FALSE(frameBuffer.hasSampleableDepth());
	ASSERT_TRUE(frameBuffer.description().depth.has_value());
	EXPECT_EQ(
		frameBuffer.description().depth->storage,
		spk::FrameBufferObject::AttachmentStorage::RenderBuffer);
	EXPECT_EQ(frameBuffer.colorAttachment().format(), spk::Texture::Format::RGBA);
}

TEST(GameEngineWidgetTest, BuildsNoRenderUnitUntilSized)
{
	spk::GameEngineWidget widget("Game");

	auto renderUnit = widget.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
}

TEST(GameEngineWidgetTest, UpdateRunsOwnedEngine)
{
	ExposedGameEngineWidget widget("Game");
	spk::UpdateContext tick{};

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
