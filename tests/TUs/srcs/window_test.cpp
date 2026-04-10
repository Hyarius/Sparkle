#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "spk_window_registry.hpp"
#include "window_test_utils.hpp"

TEST(WindowTest, ConstructionActivatesRootWidgetAndExposesAccessors)
{
	auto bundle = sparkle_test::createWindowBundle(sparkle_test::defaultRect(), "Runtime");

	ASSERT_NE(bundle.window, nullptr);
	EXPECT_TRUE(bundle.window->rootWidget().isActivated());
	EXPECT_EQ(bundle.window->rootWidget().name(), ":/Runtime/RootWidget");
	EXPECT_EQ(bundle.window->host().title(), "Runtime");
	EXPECT_EQ(bundle.window->host().rect(), sparkle_test::defaultRect());
	EXPECT_EQ(bundle.window->mouse().position, spk::Vector2Int(0, 0));
	EXPECT_EQ(bundle.window->keyboard().glyph, U'\0');
}

TEST(WindowTest, PollEventsDrivesWindowModulesAndWidgetTree)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();

	const spk::Rect2D resizedRect(0, 0, 500, 300);
	bundle.platformRuntime->queueFramePayload(spk::WindowResizedPayload{
		.rect = resizedRect});
	bundle.platformRuntime->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(8, 10)});
	bundle.platformRuntime->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(7, 9)});
	bundle.platformRuntime->queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::C,
		.isRepeated = false});

	bundle.window->pollEvents();
	bundle.window->update();

	EXPECT_EQ(bundle.platformRuntime->pollEventsCount, 1);
	EXPECT_EQ(bundle.window->mouse().position, spk::Vector2Int(7, 9));
	EXPECT_EQ(bundle.window->mouse().deltaPosition, spk::Vector2Int(-1, -1));
	EXPECT_EQ(bundle.window->keyboard()[spk::Keyboard::C], spk::InputState::Down);
	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->lastResizeRect, resizedRect);
	EXPECT_EQ(child.frameEventCount, 1);
	EXPECT_EQ(child.mouseEventCount, 2);
	EXPECT_EQ(child.keyboardEventCount, 1);
}

TEST(WindowTest, UpdateDelegatesToRootWidgetTree)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();

	bundle.window->update();

	EXPECT_EQ(child.updateCount, 1);
	EXPECT_EQ(child.lastTickMouse, &bundle.window->mouse());
	EXPECT_EQ(child.lastTickKeyboard, &bundle.window->keyboard());
}

TEST(WindowTest, RenderMakesContextCurrentExecutesSnapshotAndPresents)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();

	bundle.window->update();
	bundle.window->render();

	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->makeCurrentCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->presentCount, 1);
	EXPECT_EQ(child.renderCount, 1);
}

TEST(WindowTest, RequestClosureDelegatesToManagedWindow)
{
	auto bundle = sparkle_test::createWindowBundle();

	bundle.window->requestClosure();

	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->requestClosureCount, 1);
}

TEST(WindowTest, ClosureSubscribersAreTriggeredByValidatedCloseEvents)
{
	auto bundle = sparkle_test::createWindowBundle();
	int closureCount = 0;

	auto contract = bundle.window->subscribeToClosure([&](spk::Window* p_window)
	{
		++closureCount;
		EXPECT_EQ(p_window, bundle.window.get());
	});

	bundle.platformRuntime->queueFramePayload(spk::WindowCloseValidatedPayload{});
	bundle.window->pollEvents();
	bundle.window->update();

	EXPECT_EQ(closureCount, 1);
}