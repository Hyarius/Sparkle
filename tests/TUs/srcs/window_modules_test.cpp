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

	spk::MouseEventRecord firstMove = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(36, 45),
		.delta = spk::Vector2Int(4, 5)}));
	spk::MouseEventRecord secondMove = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(40, 50),
		.delta = spk::Vector2Int(4, 5)}));
	spk::MouseEventRecord wheel = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseWheelScrolledRecord{
		.delta = spk::Vector2(0.0f, 1.5f)}));
	spk::MouseEventRecord press = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	spk::MouseEventRecord release = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{
		.button = spk::Mouse::Left}));

	module.pushEvent(firstMove);
	module.pushEvent(secondMove);
	module.pushEvent(wheel);
	module.pushEvent(press);
	module.pushEvent(release);

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(40, 50));
	EXPECT_EQ(module.mouse().deltaPosition, spk::Vector2Int(4, 5));
	EXPECT_FLOAT_EQ(module.mouse().wheel, 1.5f);
	EXPECT_EQ(module.mouse()[spk::Mouse::Left], spk::InputState::Up);
	EXPECT_EQ(widget.mouseEventCount, 5);
}

TEST(MouseModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::MouseModule module;

	spk::MouseEventRecord event = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(9, 9),
		.delta = spk::Vector2Int(1, 1)}));

	EXPECT_NO_THROW(module.pushEvent(event));

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(0, 0));
}

TEST(KeyboardModuleTest, KeyboardEventsUpdateInternalStateAndDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("KeyboardWidget");
	widget.activate();

	spk::KeyboardModule module;
	module.bind(&widget);

	spk::KeyboardEventRecord press = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{
		.key = spk::Keyboard::Escape,
		.isRepeated = false}));
	spk::KeyboardEventRecord text = spk::KeyboardEventRecord(spk::makeEventRecord(spk::TextInputRecord{
		.glyph = U'Q'}));
	spk::KeyboardEventRecord release = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyReleasedRecord{
		.key = spk::Keyboard::Escape}));

	module.pushEvent(press);
	module.pushEvent(text);
	module.pushEvent(release);

	EXPECT_EQ(module.keyboard()[spk::Keyboard::Escape], spk::InputState::Up);
	EXPECT_EQ(module.keyboard().glyph, U'Q');
	EXPECT_EQ(widget.keyboardEventCount, 3);
}

TEST(KeyboardModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::KeyboardModule module;

	spk::KeyboardEventRecord event = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{
		.key = spk::Keyboard::A,
		.isRepeated = false}));

	EXPECT_NO_THROW(module.pushEvent(event));

	EXPECT_EQ(module.keyboard()[spk::Keyboard::A], spk::InputState::Up);
}

TEST(FrameModuleTest, FrameEventsDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("FrameWidget");
	widget.activate();

	spk::FrameModule module;
	module.bind(&widget);

	const spk::Rect2D resizedRect(0, 0, 1920, 1080);
	spk::FrameEventRecord event = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowResizedRecord{
		.rect = resizedRect}));
	module.pushEvent(event);

	EXPECT_EQ(widget.geometry(), resizedRect.atOrigin());
	EXPECT_EQ(widget.frameEventCount, 1);
	ASSERT_EQ(widget.frameEventKinds.size(), 1u);
	EXPECT_EQ(widget.frameEventKinds[0], "WindowResized");
}

TEST(FrameModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::FrameModule module;
	const spk::Rect2D resizedRect(0, 0, 1920, 1080);

	spk::FrameEventRecord event = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowResizedRecord{
		.rect = resizedRect}));

	EXPECT_NO_THROW(module.pushEvent(event));
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
