#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#define private public
#include "structures/system/device/window/spk_window_registry.hpp"
#undef private
#include "structures/system/device/window/window_test_utils.hpp"

TEST(WindowRegistryTest, CreateWindowReturnsHandleAndRejectsDuplicateIds)
{
	spk::WindowRegistry registry;

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	spk::WindowHandle windowHandle = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::shared_ptr<spk::Window> window = registry._windows.at("main").window;
	ASSERT_NE(window, nullptr);
	EXPECT_EQ(sparkle_test::WindowAccess::host(*window).title(), "Main");

	auto duplicatePlatformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();

	EXPECT_THROW(
		registry.createWindow(
			"main",
			duplicatePlatformRuntime,
			spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "MainDuplicate"
			}),
		std::runtime_error);
}

TEST(WindowRegistryTest, RequestWindowClosingDelegatesToStoredWindow)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();

	spk::WindowHandle window = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	registry.requestWindowClosing("main");

	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	std::shared_ptr<spk::Window> internalWindow = registry._windows.at("main").window;
	ASSERT_NE(internalWindow, nullptr);
	internalWindow->executePendingPlatformActions();
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 1);
}

TEST(WindowRegistryTest, WindowHandleQueuesThreadBoundCommands)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();

	spk::WindowHandle windowHandle = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	const spk::Rect2D resizedRect(1, 2, 640, 480);

	windowHandle.setTitle("Updated");
	windowHandle.resize(resizedRect);
	windowHandle.requestClosure();
	windowHandle.setVSync(true);

	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->createdFrame->currentTitle, "Main");
	EXPECT_EQ(platformRuntimePtr->createdFrame->currentRect, sparkle_test::defaultRect());
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 0);

	std::shared_ptr<spk::Window> window = registry._windows.at("main").window;
	ASSERT_NE(window, nullptr);
	window->executePendingPlatformActions();

	EXPECT_EQ(platformRuntimePtr->createdFrame->currentTitle, "Updated");
	EXPECT_EQ(platformRuntimePtr->createdFrame->currentRect, resizedRect);
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 1);

	window->render();

	ASSERT_NE(platformRuntime->createdContext, nullptr);
	EXPECT_EQ(platformRuntime->createdContext->setVSyncCount, 1);
	EXPECT_TRUE(platformRuntime->createdContext->lastVSync);
}

TEST(WindowRegistryTest, WindowHandleReportsValidityAndClosedState)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();

	spk::WindowHandle windowHandle = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	EXPECT_TRUE(windowHandle.isValid());
	EXPECT_FALSE(windowHandle.expired());
	EXPECT_FALSE(windowHandle.isClosed());

	std::shared_ptr<spk::Window> window = registry._windows.at("main").window;
	ASSERT_NE(window, nullptr);

	platformRuntime->queueFrameEvent(spk::WindowDestroyedRecord{});
	platformRuntime->pollEvents();
	window->update();
	window->render();
	window->executePendingPlatformActions();

	EXPECT_TRUE(windowHandle.isClosed());
}

TEST(WindowRegistryTest, WindowHandleExposesCentralWidgetForStackAllocatedWidgets)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();

	spk::WindowHandle windowHandle = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::shared_ptr<spk::Window> window = registry._windows.at("main").window;
	ASSERT_NE(window, nullptr);

	sparkle_test::RecordingWidget widget("Registered", &windowHandle.centralWidget());

	EXPECT_EQ(&windowHandle.centralWidget(), &sparkle_test::WindowAccess::rootWidget(*window));
	EXPECT_EQ(widget.parent(), &sparkle_test::WindowAccess::rootWidget(*window));
	EXPECT_TRUE(sparkle_test::WindowAccess::rootWidget(*window).hasChild(&widget));

	widget.activate();
	window->update();
	window->render();

	EXPECT_EQ(widget.updateCount, 1);
	EXPECT_EQ(widget.renderCount, 1);
}

TEST(WindowRegistryTest, WindowHandleCentralWidgetSupportsCallerOwnedHeapWidgets)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();

	spk::WindowHandle windowHandle = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::shared_ptr<spk::Window> window = registry._windows.at("main").window;
	ASSERT_NE(window, nullptr);

	auto widget = std::make_unique<sparkle_test::RecordingWidget>("Registered", &windowHandle.centralWidget());
	widget->activate();

	window->update();
	window->render();

	EXPECT_EQ(widget->parent(), &sparkle_test::WindowAccess::rootWidget(*window));
	EXPECT_EQ(widget->updateCount, 1);
	EXPECT_EQ(widget->renderCount, 1);
}

TEST(WindowRegistryTest, RequestWindowClosingThrowsForUnknownWindow)
{
	spk::WindowRegistry registry;

	EXPECT_THROW(registry.requestWindowClosing("missing"), std::runtime_error);
}

TEST(WindowRegistryTest, RemovedWindowsExpireExternalHandles)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();

	spk::WindowHandle windowHandle = registry.createWindow(
		"main",
		platformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	{
		std::shared_ptr<spk::Window> window = registry._windows.at("main").window;
		ASSERT_NE(window, nullptr);

		platformRuntime->queueFrameEvent(spk::WindowDestroyedRecord{});
		platformRuntime->pollEvents();
		window->update();
		window->render();
		window->executePendingPlatformActions();
	}

	registry.removeClosedWindows();

	EXPECT_TRUE(windowHandle.expired());
	EXPECT_FALSE(registry.contains("main"));
}

TEST(WindowRegistryTest, ExpiredWindowHandleReportsClosedAndIgnoresCommands)
{
	spk::WindowHandle windowHandle;

	EXPECT_FALSE(windowHandle.isValid());
	EXPECT_TRUE(windowHandle.expired());
	EXPECT_TRUE(windowHandle.isClosed());

	EXPECT_NO_THROW(windowHandle.requestClosure());
	EXPECT_NO_THROW(windowHandle.setTitle("Ignored"));
	EXPECT_NO_THROW(windowHandle.resize(sparkle_test::defaultRect()));
	EXPECT_NO_THROW(windowHandle.setVSync(true));
	EXPECT_THROW(static_cast<void>(windowHandle.centralWidget()), std::runtime_error);
}




