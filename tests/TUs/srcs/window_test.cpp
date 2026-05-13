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
	EXPECT_EQ(bundle.window->rootWidget().geometry(), sparkle_test::defaultRect().atOrigin());
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
	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = resizedRect});
	bundle.platformRuntime->queueMouseEvent(spk::MouseMovedRecord{
		.position = spk::Vector2Int(8, 10)});
	bundle.platformRuntime->queueMouseEvent(spk::MouseMovedRecord{
		.position = spk::Vector2Int(7, 9)});
	bundle.platformRuntime->queueKeyboardEvent(spk::KeyPressedRecord{
		.key = spk::Keyboard::C,
		.isRepeated = false});

	bundle.platformRuntime->pollEvents();
	bundle.window->update();

	EXPECT_EQ(bundle.platformRuntime->pollEventsCount, 1);
	EXPECT_EQ(bundle.window->rootWidget().geometry(), resizedRect.atOrigin());
	EXPECT_EQ(bundle.window->mouse().position, spk::Vector2Int(7, 9));
	EXPECT_EQ(bundle.window->mouse().deltaPosition, spk::Vector2Int(-1, -1));
	EXPECT_EQ(bundle.window->keyboard()[spk::Keyboard::C], spk::InputState::Down);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 0);
	bundle.window->render();
	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 1);
	ASSERT_NE(bundle.gpuPlatformRuntime->createdContext, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->lastResizeRect, resizedRect);
	EXPECT_EQ(child.frameEventCount, 1);
	EXPECT_EQ(child.mouseEventCount, 2);
	EXPECT_EQ(child.keyboardEventCount, 1);
}

TEST(WindowTest, UpdatePreservesCrossCategoryEventOrdering)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();

	bundle.platformRuntime->queueMouseEvent(spk::MouseMovedRecord{
		.position = spk::Vector2Int(5, 7)});
	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = spk::Rect2D(1, 2, 300, 200)});
	bundle.platformRuntime->queueKeyboardEvent(spk::KeyPressedRecord{
		.key = spk::Keyboard::A,
		.isRepeated = false});

	bundle.platformRuntime->pollEvents();
	bundle.window->update();

	ASSERT_EQ(child.callLog.size(), 5u);
	EXPECT_EQ(child.callLog[0], "Child:mouse:MouseMoved");
	EXPECT_EQ(child.callLog[1], "Child:frame:WindowResized");
	EXPECT_EQ(child.callLog[2], "Child:keyboard:KeyPressed");
	EXPECT_EQ(child.callLog[3], "Child:update");
	EXPECT_EQ(child.callLog[4], "Child:append_render");
}

TEST(WindowTest, RenderConsumesTheLatestPendingResizeAfterUpdate)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();

	const spk::Rect2D firstRect(0, 0, 500, 300);
	const spk::Rect2D secondRect(10, 20, 800, 600);

	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = firstRect});
	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = secondRect});

	bundle.platformRuntime->pollEvents();
	bundle.window->update();

	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 0);
	EXPECT_EQ(child.frameEventCount, 2);

	bundle.window->render();

	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 1);
	ASSERT_NE(bundle.gpuPlatformRuntime->createdContext, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->lastResizeRect, secondRect);

	bundle.window->render();

	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->notifyResizeCount, 1);
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

	ASSERT_NE(bundle.gpuPlatformRuntime->createdContext, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->makeCurrentCount, 1);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->presentCount, 1);
	EXPECT_EQ(child.renderCount, 1);
}

TEST(WindowTest, RequestClosureDelegatesToManagedWindow)
{
	auto bundle = sparkle_test::createWindowBundle();

	bundle.window->requestClosure();

	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->requestClosureCount, 1);
}

TEST(WindowTest, UnconsumedCloseRequestValidatesClosure)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();

	bundle.platformRuntime->queueFrameEvent(spk::WindowCloseRequestedRecord{});
	bundle.platformRuntime->pollEvents();
	bundle.window->update();
	bundle.window->executePendingPlatformActions();

	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->validateClosureCount, 1);
	ASSERT_EQ(child.frameEventKinds.size(), 1u);
	EXPECT_EQ(child.frameEventKinds[0], "WindowCloseRequested");
}

TEST(WindowTest, ConsumedCloseRequestDoesNotValidateClosure)
{
	auto bundle = sparkle_test::createWindowBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.window->rootWidget());
	child.activate();
	child.consumeFrameEvent = true;

	bundle.platformRuntime->queueFrameEvent(spk::WindowCloseRequestedRecord{});
	bundle.platformRuntime->pollEvents();
	bundle.window->update();
	bundle.window->executePendingPlatformActions();

	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->validateClosureCount, 0);
	EXPECT_EQ(child.frameEventCount, 1);
}

TEST(WindowTest, ClosureSubscribersAreTriggeredByDestroyedEvents)
{
	auto bundle = sparkle_test::createWindowBundle();
	int closureCount = 0;

	auto contract = bundle.window->subscribeToClosure([&](spk::Window* p_window)
	{
		++closureCount;
		EXPECT_EQ(p_window, bundle.window.get());
	});

	bundle.platformRuntime->queueFrameEvent(spk::WindowDestroyedRecord{});
	bundle.platformRuntime->pollEvents();
	bundle.window->update();
	bundle.window->executePendingPlatformActions();

	EXPECT_EQ(closureCount, 1);
}
