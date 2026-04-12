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
	sparkle_test::RecordingWidget widget("MouseWidget");
	widget.activate();

	spk::MouseModule module;
	module.bind(&widget);

	module.pushEvent(spk::Event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(36, 45),
		.delta = spk::Vector2Int(4, 5)}));
	module.pushEvent(spk::Event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(40, 50),
		.delta = spk::Vector2Int(4, 5)}));
	module.pushEvent(spk::Event(spk::MouseWheelScrolledPayload{
		.delta = spk::Vector2(0.0f, 1.5f)}));
	module.pushEvent(spk::Event(spk::MouseButtonPressedPayload{
		.button = spk::Mouse::Left}));
	module.pushEvent(spk::Event(spk::MouseButtonReleasedPayload{
		.button = spk::Mouse::Left}));

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(40, 50));
	EXPECT_EQ(module.mouse().deltaPosition, spk::Vector2Int(4, 5));
	EXPECT_FLOAT_EQ(module.mouse().wheel, 1.5f);
	EXPECT_EQ(module.mouse()[spk::Mouse::Left], spk::InputState::Up);
	EXPECT_EQ(widget.mouseEventCount, 5);
}

TEST(MouseModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::MouseModule module;

	EXPECT_NO_THROW(module.pushEvent(spk::Event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(9, 9),
		.delta = spk::Vector2Int(1, 1)})));

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(0, 0));
}

TEST(KeyboardModuleTest, KeyboardEventsUpdateInternalStateAndDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("KeyboardWidget");
	widget.activate();

	spk::KeyboardModule module;
	module.bind(&widget);

	module.pushEvent(spk::Event(spk::KeyPressedPayload{
		.key = spk::Keyboard::Escape,
		.isRepeated = false}));
	module.pushEvent(spk::Event(spk::TextInputPayload{
		.glyph = U'Q'}));
	module.pushEvent(spk::Event(spk::KeyReleasedPayload{
		.key = spk::Keyboard::Escape}));

	EXPECT_EQ(module.keyboard()[spk::Keyboard::Escape], spk::InputState::Up);
	EXPECT_EQ(module.keyboard().glyph, U'Q');
	EXPECT_EQ(widget.keyboardEventCount, 3);
}

TEST(KeyboardModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::KeyboardModule module;

	EXPECT_NO_THROW(module.pushEvent(spk::Event(spk::KeyPressedPayload{
		.key = spk::Keyboard::A,
		.isRepeated = false})));

	EXPECT_EQ(module.keyboard()[spk::Keyboard::A], spk::InputState::Up);
}

TEST(FrameModuleTest, FrameEventsDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("FrameWidget");
	widget.activate();

	spk::FrameModule module;
	module.bind(&widget);

	const spk::Rect2D resizedRect(0, 0, 1920, 1080);
	module.pushEvent(spk::Event(spk::WindowResizedPayload{
		.rect = resizedRect}));

	EXPECT_EQ(widget.geometry(), resizedRect.atOrigin());
	EXPECT_EQ(widget.frameEventCount, 1);
	ASSERT_EQ(widget.frameEventKinds.size(), 1u);
	EXPECT_EQ(widget.frameEventKinds[0], "WindowResized");
}

TEST(FrameModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::FrameModule module;
	const spk::Rect2D resizedRect(0, 0, 1920, 1080);

	EXPECT_NO_THROW(module.pushEvent(spk::Event(spk::WindowResizedPayload{
		.rect = resizedRect})));
}

TEST(UpdateModuleTest, UpdateIsSafeWhenUnbound)
{
	spk::UpdateModule module;

	EXPECT_NO_THROW(module.update());
}

TEST(UpdateModuleTest, UpdateBuildsAndForwardsTickToBoundWidget)
{
	spk::UpdateModule module;
	sparkle_test::RecordingWidget widget("UpdateWidget");

	widget.activate();
	module.bind(&widget);

	module.update();

	EXPECT_EQ(widget.updateCount, 1);
}

TEST(UpdateModuleTest, BoundInputsAreInjectedIntoConstructedTick)
{
	spk::UpdateModule module;
	sparkle_test::RecordingWidget widget("UpdateWidget");
	spk::Mouse mouse;
	spk::Keyboard keyboard;

	widget.activate();
	module.bind(&widget);
	module.bindInputs(&mouse, &keyboard);

	module.update();

	EXPECT_EQ(widget.updateCount, 1);
	EXPECT_EQ(widget.lastTickMouse, &mouse);
	EXPECT_EQ(widget.lastTickKeyboard, &keyboard);
}

TEST(RenderModuleTest, RenderExecutesCommandsAndIsSafeWithEmptyBuilder)
{
	spk::RenderModule module;
	spk::RenderCommandBuilder builder;

	EXPECT_NO_THROW(module.render(builder));

	sparkle_test::RecordingWidget widget("RenderWidget");
	widget.activate();

	widget.appendRenderCommands(builder);
	module.render(builder);

	EXPECT_EQ(widget.appendRenderCommandCount, 1);
	EXPECT_EQ(widget.renderCount, 1);
}
