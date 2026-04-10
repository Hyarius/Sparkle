#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "spk_application.hpp"
#include "window_test_utils.hpp"

TEST(ApplicationTest, CreateWindowAndLookupReturnTheManagedWindow)
{
	spk::Application application;

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	auto* platformRuntimePtr = platformRuntime.get();

	spk::Window& window = application.createWindow(
		"main",
		spk::WindowHost::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

	EXPECT_TRUE(application.containsWindow("main"));
	EXPECT_EQ(&application.window("main"), &window);
	EXPECT_EQ(window.host().title(), "Main");
	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->lastCreateTitle, "Main");
}

TEST(ApplicationTest, RequestWindowClosingDelegatesToTheManagedWindow)
{
	spk::Application application;

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	auto* platformRuntimePtr = platformRuntime.get();

	application.createWindow(
		"main",
		spk::WindowHost::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

	application.requestWindowClosing("main");

	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 1);
}

TEST(ApplicationTest, RunExecutesEventUpdateAndRenderLoops)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);

	spk::Application application(
		spk::Application::Configuration{
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step,
			.stopWhenNoWindows = false
		});

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	auto* platformRuntimePtr = platformRuntime.get();
	auto* renderBackendPtr = renderBackend.get();

	spk::Window& window = application.createWindow(
		"main",
		spk::WindowHost::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

	std::atomic<int> updateCount = 0;
	std::atomic<int> renderCount = 0;

	sparkle_test::CallbackWidget probe("Probe", &window.rootWidget());
	probe.activate();
	probe.onUpdate = [&](const spk::UpdateTick& p_tick)
	{
		++updateCount;
		EXPECT_NE(p_tick.mouse, nullptr);
		EXPECT_NE(p_tick.keyboard, nullptr);
	};
	probe.onRender = [&]()
	{
		++renderCount;
	};

	platformRuntimePtr->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(11, 13),
		.delta = spk::Vector2Int(0, 0)});
	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		if (updateCount.load() > 100 && renderCount.load() > 100)
		{
			application.stop();
		}
	};

	application.run();

	EXPECT_GT(platformRuntimePtr->pollEventsCount, 0);
	EXPECT_GT(updateCount.load(), 0);
	EXPECT_GT(renderCount.load(), 0);
	EXPECT_EQ(window.mouse().position, spk::Vector2Int(11, 13));
	ASSERT_NE(renderBackendPtr->createdContext, nullptr);
	EXPECT_GT(renderBackendPtr->createdContext->makeCurrentCount, 0);
	EXPECT_GT(renderBackendPtr->createdContext->presentCount, 0);
}

TEST(ApplicationTest, RunStopsWhenTheLastWindowCloses)
{
	spk::Application application;

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	auto* platformRuntimePtr = platformRuntime.get();

	application.createWindow(
		"main",
		spk::WindowHost::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.platformRuntime = platformRuntime,
			.renderBackend = std::move(renderBackend)
		});

	platformRuntimePtr->queueFramePayload(spk::WindowCloseValidatedPayload{});

	application.run();

	EXPECT_FALSE(application.containsWindow("main"));
	EXPECT_GT(platformRuntimePtr->pollEventsCount, 0);
}
