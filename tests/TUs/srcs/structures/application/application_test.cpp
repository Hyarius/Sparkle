#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#define private public
#include "structures/application/spk_application.hpp"
#undef private
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	class ThrowingPresentRenderContext : public sparkle_test::TestRenderContext
	{
	private:
		std::barrier<>* _failureBarrier = nullptr;
		std::atomic<bool>* _renderFailureReached = nullptr;

	public:
		ThrowingPresentRenderContext(
			std::shared_ptr<spk::SurfaceState> p_surfaceState,
			std::shared_ptr<sparkle_test::TestRenderContextStats> p_stats,
			std::barrier<>* p_failureBarrier,
			std::atomic<bool>* p_renderFailureReached) :
			sparkle_test::TestRenderContext(std::move(p_surfaceState), std::move(p_stats)),
			_failureBarrier(p_failureBarrier),
			_renderFailureReached(p_renderFailureReached)
		{
		}

		void present() override
		{
			_renderFailureReached->store(true);
			_failureBarrier->arrive_and_wait();
			throw std::runtime_error("render failure");
		}
	};

	class ThrowingPresentPlatformRuntime : public sparkle_test::TestPlatformRuntime
	{
	private:
		std::barrier<>* _failureBarrier = nullptr;
		std::atomic<bool>* _renderFailureReached = nullptr;

	public:
		ThrowingPresentPlatformRuntime(
			std::barrier<>* p_failureBarrier,
			std::atomic<bool>* p_renderFailureReached) :
			_failureBarrier(p_failureBarrier),
			_renderFailureReached(p_renderFailureReached)
		{
		}

		std::unique_ptr<spk::RenderContext> createRenderContext(spk::IFrame& p_frame) override
		{
			++createRenderContextCount;
			lastFrame = &p_frame;
			lastCreateRenderContextThreadID = std::this_thread::get_id();

			contextStats = std::make_shared<sparkle_test::TestRenderContextStats>();
			auto result = std::make_unique<ThrowingPresentRenderContext>(
				p_frame.surfaceState(),
				contextStats,
				_failureBarrier,
				_renderFailureReached);
			result->creationThreadID = std::this_thread::get_id();
			contextStats->creationThreadID = result->creationThreadID;
			createdContext = result.get();
			return result;
		}
	};

}

TEST(ApplicationTest, CreateWindowAndLookupReturnTheManagedWindow)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime
		});

	spk::WindowHandle windowHandle = application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	EXPECT_TRUE(application.containsWindow("main"));
	EXPECT_FALSE(application.window("main").expired());
	EXPECT_FALSE(windowHandle.expired());
	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->createdFrame->currentTitle, "Main");
	EXPECT_EQ(platformRuntimePtr->lastCreateTitle, "Main");
}

TEST(ApplicationTest, DefaultCreateWindowUsesNativeRuntimeWhenAvailable)
{
	spk::Application application;

	EXPECT_NO_THROW(
		application.createWindow(
			"main",
			spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "Main"
			}));
	EXPECT_TRUE(application.containsWindow("main"));
}

TEST(ApplicationTest, ConstWindowLookupReturnsTheManagedWindow)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	const spk::Application& constApplication = application;

	EXPECT_FALSE(constApplication.window("main").expired());
}

TEST(ApplicationTest, RequestWindowClosingDelegatesToTheManagedWindow)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	application.requestWindowClosing("main");

	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	std::shared_ptr<spk::Window> window = application._windowRegistry._windows.at("main").window;
	ASSERT_NE(window, nullptr);
	window->executePendingPlatformActions();
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 1);
}

TEST(ApplicationTest, RunExecutesEventUpdateAndRenderLoops)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	const std::thread::id ownerThreadID = std::this_thread::get_id();

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	auto* platformRuntimePtr = platformRuntime.get();
	auto* gpuPlatformRuntimePtr = gpuPlatformRuntime.get();

	spk::WindowHandle windowHandle = application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::atomic<int> updateCount = 0;
	std::atomic<int> renderCount = 0;
	std::atomic<bool> destroyQueued = false;

	std::shared_ptr<spk::Window> window = application._windowRegistry._windows.at("main").window;
	ASSERT_NE(window, nullptr);
	sparkle_test::CallbackWidget probe("Probe", &sparkle_test::WindowAccess::rootWidget(*window));
	probe.activate();
	probe.onUpdate = [&](const spk::UpdateContext& p_tick)
	{
		++updateCount;
		EXPECT_NE(p_tick.mouse, nullptr);
		EXPECT_NE(p_tick.keyboard, nullptr);
	};
	probe.onRender = [&]()
	{
		++renderCount;
	};

	platformRuntimePtr->queueMouseEvent(spk::MouseMovedRecord{
		.position = spk::Vector2Int(11, 13),
		.delta = spk::Vector2Int(0, 0)});
	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		if (destroyQueued.load() == false &&
			updateCount.load() > 10 &&
			renderCount.load() > 10)
		{
			destroyQueued.store(true);
			platformRuntimePtr->queueFrameEvent(spk::WindowDestroyedRecord{});
		}
	};

	EXPECT_EQ(application.run(), 0);

	EXPECT_GT(platformRuntimePtr->pollEventsCount, 0);
	EXPECT_GT(updateCount.load(), 0);
	EXPECT_GT(renderCount.load(), 0);
	EXPECT_EQ(sparkle_test::WindowAccess::mouse(*window).position, spk::Vector2Int(11, 13));
	ASSERT_NE(gpuPlatformRuntimePtr->contextStats, nullptr);
	EXPECT_GT(gpuPlatformRuntimePtr->contextStats->makeCurrentCount, 0);
	EXPECT_GT(gpuPlatformRuntimePtr->contextStats->presentCount, 0);
	EXPECT_EQ(gpuPlatformRuntimePtr->contextStats->creationThreadID, gpuPlatformRuntimePtr->lastCreateRenderContextThreadID);
	EXPECT_EQ(gpuPlatformRuntimePtr->contextStats->lastMakeCurrentThreadID, gpuPlatformRuntimePtr->contextStats->lastPresentThreadID);
	EXPECT_NE(gpuPlatformRuntimePtr->lastCreateRenderContextThreadID, ownerThreadID);
	EXPECT_FALSE(application.containsWindow("main"));
}

TEST(ApplicationTest, RunExecutesDeferredFrameValidationOnTheOwnerThread)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	const std::thread::id ownerThreadID = std::this_thread::get_id();
	std::atomic<bool> destroyQueued = false;

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	platformRuntimePtr->queueFrameEvent(spk::WindowCloseRequestedRecord{});
	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		if (platformRuntimePtr->createdFrame != nullptr &&
			platformRuntimePtr->createdFrame->validateClosureCount > 0 &&
			destroyQueued.load() == false)
		{
			destroyQueued.store(true);
			platformRuntimePtr->queueFrameEvent(spk::WindowDestroyedRecord{});
		}
	};

	EXPECT_EQ(application.run(), 0);

	ASSERT_NE(platformRuntimePtr->frameStats, nullptr);
	EXPECT_EQ(platformRuntimePtr->frameStats->validateClosureCount, 1);
	EXPECT_EQ(platformRuntimePtr->frameStats->lastValidateClosureThreadID, ownerThreadID);
}

TEST(ApplicationTest, RunRethrowsWorkerThreadFailuresOnTheCallerThread)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	spk::WindowHandle windowHandle = application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::shared_ptr<spk::Window> window = application._windowRegistry._windows.at("main").window;
	ASSERT_NE(window, nullptr);

	sparkle_test::CallbackWidget probe("Probe", &sparkle_test::WindowAccess::rootWidget(*window));
	probe.activate();
	probe.onUpdate = [](const spk::UpdateContext&)
	{
		throw std::runtime_error("update failure");
	};

	EXPECT_THROW(application.run(), std::runtime_error);
	EXPECT_FALSE(application.isRunning());
}

TEST(ApplicationTest, RunRethrowsOneWorkerFailureWhenRenderAndUpdateFailConcurrently)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	std::barrier failureBarrier(2);
	std::atomic<bool> updateFailureReached = false;
	std::atomic<bool> renderFailureReached = false;
	auto gpuPlatformRuntime = std::make_shared<ThrowingPresentPlatformRuntime>(
		&failureBarrier,
		&renderFailureReached);

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = gpuPlatformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::shared_ptr<spk::Window> window = application._windowRegistry._windows.at("main").window;
	ASSERT_NE(window, nullptr);

	sparkle_test::CallbackWidget probe("Probe", &sparkle_test::WindowAccess::rootWidget(*window));
	probe.activate();
	probe.onUpdate = [&](const spk::UpdateContext&)
	{
		updateFailureReached.store(true);
		failureBarrier.arrive_and_wait();
		throw std::runtime_error("update failure");
	};

	try
	{
		application.run();
		FAIL() << "Expected one worker exception to be rethrown";
	}
	catch (const std::runtime_error& exception)
	{
		const std::string message = exception.what();
		EXPECT_TRUE(message == "update failure" || message == "render failure");
	}

	EXPECT_TRUE(updateFailureReached.load());
	EXPECT_TRUE(renderFailureReached.load());
	EXPECT_FALSE(application.isRunning());
	EXPECT_EQ(application._failure, nullptr);
}

TEST(ApplicationTest, ExternalQuitGracefullyClosesWindows)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	std::atomic<bool> sawClosureRequest = false;
	std::atomic<bool> destroyQueued = false;

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	std::jthread externalQuitThread([&]()
	{
		while (application.isRunning() == false)
		{
			std::this_thread::yield();
		}

		application.quit(17);
	});

	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		if (destroyQueued.load() == false &&
			platformRuntimePtr->createdFrame != nullptr &&
			platformRuntimePtr->createdFrame->requestClosureCount > 0)
		{
			sawClosureRequest.store(true);
			destroyQueued.store(true);
			platformRuntimePtr->queueFrameEvent(spk::WindowDestroyedRecord{});
		}
	};

	EXPECT_EQ(application.run(), 17);
	externalQuitThread.join();

	EXPECT_TRUE(sawClosureRequest.load());
	EXPECT_FALSE(application.containsWindow("main"));
}

TEST(ApplicationTest, StopRequestsClosureAndWaitsForWindowDisposal)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	std::atomic<bool> stopIssued = false;
	std::atomic<bool> sawClosureRequest = false;
	std::atomic<bool> destroyQueued = false;

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		if (stopIssued.load() == false)
		{
			stopIssued.store(true);
			application.stop();
			return;
		}

		if (destroyQueued.load() == false &&
			platformRuntimePtr->createdFrame != nullptr &&
			platformRuntimePtr->createdFrame->requestClosureCount > 0)
		{
			sawClosureRequest.store(true);
			destroyQueued.store(true);
			platformRuntimePtr->queueFrameEvent(spk::WindowDestroyedRecord{});
		}
	};

	EXPECT_EQ(application.run(), 0);

	EXPECT_TRUE(stopIssued.load());
	EXPECT_TRUE(sawClosureRequest.load());
	EXPECT_FALSE(application.containsWindow("main"));
}

TEST(ApplicationTest, QuitRequestsClosureAndReturnsTheProvidedExitCode)
{
	const spk::Duration step(1.0L, spk::TimeUnit::Millisecond);
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	std::atomic<bool> quitIssued = false;
	std::atomic<bool> sawClosureRequest = false;
	std::atomic<bool> destroyQueued = false;

	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime,
			.renderInterval = step,
			.updateInterval = step,
			.eventPollingInterval = step
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	platformRuntimePtr->onPollEvents = [&](sparkle_test::TestPlatformRuntime&)
	{
		if (quitIssued.load() == false)
		{
			quitIssued.store(true);
			application.quit(42);
			return;
		}

		if (destroyQueued.load() == false &&
			platformRuntimePtr->createdFrame != nullptr &&
			platformRuntimePtr->createdFrame->requestClosureCount > 0)
		{
			sawClosureRequest.store(true);
			destroyQueued.store(true);
			platformRuntimePtr->queueFrameEvent(spk::WindowDestroyedRecord{});
		}
	};

	EXPECT_EQ(application.run(), 42);

	EXPECT_TRUE(quitIssued.load());
	EXPECT_TRUE(sawClosureRequest.load());
	EXPECT_FALSE(application.containsWindow("main"));
}

TEST(ApplicationTest, RunStopsWhenTheLastWindowCloses)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	auto* platformRuntimePtr = platformRuntime.get();
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime
		});

	application.createWindow(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	platformRuntimePtr->queueFrameEvent(spk::WindowDestroyedRecord{});

	EXPECT_EQ(application.run(), 0);

	EXPECT_FALSE(application.containsWindow("main"));
	EXPECT_GT(platformRuntimePtr->pollEventsCount, 0);
}

TEST(ApplicationTest, RunReturnsImmediatelyWhenNoWindows)
{
	spk::Application application;

	EXPECT_EQ(application.run(), 0);

	EXPECT_FALSE(application.isRunning());
}

TEST(ApplicationTest, RunThrowsWhenApplicationIsAlreadyRunning)
{
	spk::Application application(
		spk::Application::Configuration{});

	application._isRunning.store(true);

	EXPECT_THROW(application.run(), std::runtime_error);

	application._isRunning.store(false);
}

TEST(ApplicationTest, RunThrowsWhenCalledFromDifferentThreadThanApplicationConstruction)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = platformRuntime;
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime
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
	auto gpuPlatformRuntime = platformRuntime;
	spk::Application application(
		spk::Application::Configuration{
			.platformRuntime = platformRuntime
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



