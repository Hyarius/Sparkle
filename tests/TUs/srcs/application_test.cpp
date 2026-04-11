#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "spk_application.hpp"
#include "window_test_utils.hpp"

TEST(ApplicationTest, CreateWindowAndLookupReturnTheManagedWindow)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime
		});

	std::shared_ptr<spk::Window> window = application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	EXPECT_TRUE(application.containsWindow("main"));
	EXPECT_EQ(application.window("main"), window);
	ASSERT_NE(window, nullptr);
	EXPECT_EQ(window->host().title(), "Main");
	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->lastCreateTitle, "Main");
}

TEST(ApplicationTest, RequestWindowClosingDelegatesToTheManagedWindow)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	application.requestWindowClosing("main");

	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 1);
}

TEST(ApplicationTest, RunExecutesEventUpdateAndRenderLoops)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step,
			.stopWhenNoWindows = false
		});

	auto* platformRuntimePtr = platformRuntime.get();
	auto* gpuPlatformRuntimePtr = gpuPlatformRuntime.get();

	std::shared_ptr<spk::Window> window = application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::atomic<int> updateCount = 0;
	std::atomic<int> renderCount = 0;

	ASSERT_NE(window, nullptr);
	sparkle_test::CallbackWidget probe("Probe", &window->rootWidget());
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
		if (updateCount.load() > 10 && renderCount.load() > 10)
		{
			application.stop();
		}
	};

	application.run();

	EXPECT_GT(platformRuntimePtr->pollEventsCount, 0);
	EXPECT_GT(updateCount.load(), 0);
	EXPECT_GT(renderCount.load(), 0);
	EXPECT_EQ(window->mouse().position, spk::Vector2Int(11, 13));
	ASSERT_NE(gpuPlatformRuntimePtr->createdContext, nullptr);
	EXPECT_GT(gpuPlatformRuntimePtr->createdContext->makeCurrentCount, 0);
	EXPECT_GT(gpuPlatformRuntimePtr->createdContext->presentCount, 0);
}

TEST(ApplicationTest, RunRethrowsWorkerThreadFailuresOnTheCallerThread)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step,
			.stopWhenNoWindows = false
		});

	std::shared_ptr<spk::Window> window = application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	ASSERT_NE(window, nullptr);

	sparkle_test::CallbackWidget probe("Probe", &window->rootWidget());
	probe.activate();
	probe.onUpdate = [](const spk::UpdateTick&)
	{
		throw std::runtime_error("update failure");
	};

	EXPECT_THROW(application.run(), std::runtime_error);
	EXPECT_FALSE(application.isRunning());
}

TEST(ApplicationTest, RunPollsTheSharedPlatformRuntimeOncePerIteration)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step,
			.stopWhenNoWindows = false
		});

	application.createWindow(
		"first",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "First"
		});

	application.createWindow(
		"second",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Second"
		});

	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		application.stop();
	};

	application.run();

	EXPECT_EQ(platformRuntimePtr->pollEventsCount, 1);
}

TEST(ApplicationTest, RunStopsWhenTheLastWindowCloses)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	platformRuntimePtr->queueFramePayload(spk::WindowCloseValidatedPayload{});

	application.run();

	EXPECT_FALSE(application.containsWindow("main"));
	EXPECT_GT(platformRuntimePtr->pollEventsCount, 0);
}

TEST(ApplicationTest, RunThrowsWhenCalledFromDifferentThreadThanApplicationConstruction)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::exception_ptr failure = nullptr;
	std::jthread worker([&]()
	{
		try
		{
			application.run();
		}
		catch (...)
		{
			failure = std::current_exception();
		}
	});
	worker.join();

	ASSERT_NE(failure, nullptr);
	EXPECT_THROW(std::rethrow_exception(failure), std::runtime_error);
}

TEST(ApplicationTest, CreateWindowThrowsWhenCalledFromDifferentThreadThanApplicationConstruction)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.gpuPlatformRuntime = gpuPlatformRuntime
		});

	std::exception_ptr failure = nullptr;
	std::jthread worker([&]()
	{
		try
		{
			application.createWindow(
				"main",
				spk::Window::Configuration{
					.rect = sparkle_test::defaultRect(),
					.title = "Main"
				});
		}
		catch (...)
		{
			failure = std::current_exception();
		}
	});
	worker.join();

	ASSERT_NE(failure, nullptr);
	EXPECT_THROW(std::rethrow_exception(failure), std::runtime_error);
	EXPECT_FALSE(application.containsWindow("main"));
}
