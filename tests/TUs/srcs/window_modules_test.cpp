#include <gtest/gtest.h>

#include "spk_window_modules.hpp"
#include "window_test_utils.hpp"

namespace
{
	class TestModule : public spk::IModule
	{
	};
}

TEST(IModuleTest, BindStoresWidgetPointerAndAllowsNull)
{
	TestModule module;
	sparkle_test::RecordingWidget widget("Widget");

	module.bind(&widget);
	EXPECT_EQ(module.widget(), &widget);

	module.bind(nullptr);
	EXPECT_EQ(module.widget(), nullptr);
}

TEST(MouseModuleTest, MouseEventsUpdateInternalStateAndDispatchToBoundWidget)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget widget("MouseWidget");
	widget.activate();

	spk::MouseModule module;
	module.bind(&widget);
	module.bindWindow(bundle.window.get());

	bundle.platformRuntime->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(40, 50),
		.delta = spk::Vector2Int(4, 5)});
	bundle.platformRuntime->queueMousePayload(spk::MouseWheelScrolledPayload{
		.delta = spk::Vector2(0.0f, 1.5f)});
	bundle.platformRuntime->queueMousePayload(spk::MouseButtonPressedPayload{
		.button = spk::Mouse::Left});
	bundle.platformRuntime->queueMousePayload(spk::MouseButtonReleasedPayload{
		.button = spk::Mouse::Left});

	bundle.window->pollEvents();

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(40, 50));
	EXPECT_EQ(module.mouse().deltaPosition, spk::Vector2Int(4, 5));
	EXPECT_FLOAT_EQ(module.mouse().wheel, 1.5f);
	EXPECT_EQ(module.mouse()[spk::Mouse::Left], spk::InputState::Up);
	EXPECT_EQ(widget.mouseEventCount, 4);
}

TEST(MouseModuleTest, BindingNullWindowResignsMouseSubscription)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget widget("MouseWidget");
	widget.activate();

	spk::MouseModule module;
	module.bind(&widget);
	module.bindWindow(bundle.window.get());
	module.bindWindow(nullptr);

	bundle.platformRuntime->emitMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(9, 9),
		.delta = spk::Vector2Int(1, 1)});

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(0, 0));
	EXPECT_EQ(widget.mouseEventCount, 0);
}

TEST(KeyboardModuleTest, KeyboardEventsUpdateInternalStateAndDispatchToBoundWidget)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget widget("KeyboardWidget");
	widget.activate();

	spk::KeyboardModule module;
	module.bind(&widget);
	module.bindWindow(bundle.window.get());

	bundle.platformRuntime->queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::Escape,
		.isRepeated = false});
	bundle.platformRuntime->queueKeyboardPayload(spk::TextInputPayload{
		.glyph = U'Q'});
	bundle.platformRuntime->queueKeyboardPayload(spk::KeyReleasedPayload{
		.key = spk::Keyboard::Escape});

	bundle.window->pollEvents();

	EXPECT_EQ(module.keyboard()[spk::Keyboard::Escape], spk::InputState::Up);
	EXPECT_EQ(module.keyboard().glyph, U'Q');
	EXPECT_EQ(widget.keyboardEventCount, 3);
}

TEST(KeyboardModuleTest, BindingNullWindowResignsKeyboardSubscription)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget widget("KeyboardWidget");
	widget.activate();

	spk::KeyboardModule module;
	module.bind(&widget);
	module.bindWindow(bundle.window.get());
	module.bindWindow(nullptr);

	bundle.platformRuntime->emitKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::A,
		.isRepeated = false});

	EXPECT_EQ(module.keyboard()[spk::Keyboard::A], spk::InputState::Up);
	EXPECT_EQ(widget.keyboardEventCount, 0);
}

TEST(FrameModuleTest, WindowResizeEventsNotifyRenderContextAndDispatchToWidget)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget widget("FrameWidget");
	widget.activate();

	spk::FrameModule module;
	module.bind(&widget);
	module.bindWindow(bundle.window.get());

	const spk::Rect2D resizedRect(0, 0, 1920, 1080);
	bundle.platformRuntime->queueFramePayload(spk::WindowResizedPayload{
		.rect = resizedRect});

	bundle.window->pollEvents();

	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->lastResizeRect, resizedRect);
	EXPECT_EQ(widget.frameEventCount, 1);
	ASSERT_EQ(widget.frameEventKinds.size(), 1u);
	EXPECT_EQ(widget.frameEventKinds[0], "WindowResized");
}

TEST(UpdateModuleTest, UpdateForwardsTickToBoundWidgetAndIsSafeWhenUnbound)
{
	spk::UpdateModule module;
	spk::UpdateTick tick;

	EXPECT_NO_THROW(module.update(tick));

	sparkle_test::RecordingWidget widget("UpdateWidget");
	widget.activate();
	tick.mouse = nullptr;
	tick.keyboard = nullptr;

	module.bind(&widget);
	module.update(tick);

	EXPECT_EQ(widget.updateCount, 1);
	EXPECT_EQ(widget.lastTickMouse, nullptr);
	EXPECT_EQ(widget.lastTickKeyboard, nullptr);
}

TEST(RenderModuleTest, RenderCallsBoundWidgetAndIsSafeWhenUnbound)
{
	spk::RenderModule module;

	EXPECT_NO_THROW(module.render());

	sparkle_test::RecordingWidget widget("RenderWidget");
	widget.activate();

	module.bind(&widget);
	module.render();

	EXPECT_EQ(widget.renderCount, 1);
}
