#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>

#include "structures/system/device/window/window_test_utils.hpp"

TEST(WindowHostTest, ConstructorRejectsMissingDependencies)
{
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto frame = std::make_unique<sparkle_test::TestFrame>(sparkle_test::defaultRect(), "Window");

	EXPECT_THROW(spk::WindowHost(nullptr, gpuPlatformRuntime), std::runtime_error);
	EXPECT_THROW(spk::WindowHost(std::move(frame), nullptr), std::runtime_error);
}

TEST(WindowHostTest, PlatformOperationsThrowFromNonPlatformThread)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	std::exception_ptr failure = nullptr;

	std::jthread worker([&]()
	{
		try
		{
			bundle.windowHost->setTitle("WrongThread");
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

TEST(WindowHostTest, PlatformOperationsThrowAfterFrameRelease)
{
	auto bundle = sparkle_test::createWindowHostBundle();

	bundle.windowHost->releaseFrame();

	EXPECT_THROW(bundle.windowHost->resize(sparkle_test::defaultRect()), std::runtime_error);
	EXPECT_THROW(bundle.windowHost->setTitle("Released"), std::runtime_error);
	EXPECT_THROW(bundle.windowHost->requestClosure(), std::runtime_error);
	EXPECT_THROW(bundle.windowHost->validateClosure(), std::runtime_error);
	EXPECT_THROW(static_cast<void>(bundle.windowHost->rect()), std::runtime_error);
	EXPECT_THROW(static_cast<void>(bundle.windowHost->title()), std::runtime_error);
	EXPECT_THROW(bundle.windowHost->subscribeToMouseEvents([](const spk::MouseEventRecord&) {}), std::runtime_error);
	EXPECT_THROW(bundle.windowHost->subscribeToKeyboardEvents([](const spk::KeyboardEventRecord&) {}), std::runtime_error);
	EXPECT_THROW(bundle.windowHost->subscribeToFrameEvents([](const spk::FrameEventRecord&) {}), std::runtime_error);
}

TEST(WindowHostTest, PresentAndRenderContextRequireAValidRenderContext)
{
	auto bundle = sparkle_test::createWindowHostBundle();

	EXPECT_THROW(bundle.windowHost->present(), std::runtime_error);

	bundle.platformRuntime->createdFrame->emitFrameEvent(
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowDestroyedRecord{})));

	EXPECT_THROW(static_cast<void>(bundle.windowHost->renderContext()), std::runtime_error);
}

TEST(WindowHostTest, NullRenderContextCreationThrows)
{
	auto bundle = sparkle_test::createWindowHostBundle();

	bundle.gpuPlatformRuntime->returnNullContext = true;

	EXPECT_THROW(static_cast<void>(bundle.windowHost->makeCurrent()), std::runtime_error);
}

TEST(WindowHostTest, ReleasingFrameInvalidatesTheExistingRenderContext)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	std::atomic<bool> contextCreated = false;
	std::atomic<bool> allowPresent = false;
	std::atomic<bool> presentAttemptFinished = false;
	std::atomic<bool> allowRelease = false;

	std::jthread renderThread([&]()
	{
		EXPECT_TRUE(bundle.windowHost->makeCurrent());
		contextCreated.store(true);

		while (allowPresent.load() == false)
		{
			std::this_thread::yield();
		}

		bundle.windowHost->present();
		presentAttemptFinished.store(true);

		while (allowRelease.load() == false)
		{
			std::this_thread::yield();
		}

		bundle.windowHost->releaseRenderContext();
	});

	while (contextCreated.load() == false)
	{
		std::this_thread::yield();
	}

	std::shared_ptr<spk::SurfaceState> surfaceState = bundle.platformRuntime->createdFrame->surfaceState();
	ASSERT_NE(bundle.gpuPlatformRuntime->createdContext, nullptr);
	EXPECT_TRUE(bundle.gpuPlatformRuntime->createdContext->isValid());
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->makeCurrentCount, 1);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->surfaceState(), bundle.platformRuntime->createdFrame->surfaceState());

	bundle.windowHost->releaseFrame();

	EXPECT_FALSE(bundle.gpuPlatformRuntime->createdContext->isValid());
	ASSERT_NE(surfaceState, nullptr);
	EXPECT_FALSE(surfaceState->isValid());

	const int presentCountBeforeInvalidPresent = bundle.gpuPlatformRuntime->createdContext->presentCount;
	allowPresent.store(true);

	while (presentAttemptFinished.load() == false)
	{
		std::this_thread::yield();
	}

	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext->presentCount, presentCountBeforeInvalidPresent);

	allowRelease.store(true);
	renderThread.join();
}

TEST(WindowHostTest, DestroyedFrameEventPreventsLateRenderContextCreation)
{
	auto bundle = sparkle_test::createWindowHostBundle();

	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 0);
	EXPECT_TRUE(bundle.platformRuntime->createdFrame->surfaceState()->isValid());

	bundle.platformRuntime->createdFrame->emitFrameEvent(
		spk::FrameEventRecord(spk::makeEventRecord(spk::WindowDestroyedRecord{})));

	EXPECT_FALSE(bundle.platformRuntime->createdFrame->surfaceState()->isValid());
	EXPECT_FALSE(bundle.windowHost->makeCurrent());
	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 0);
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext, nullptr);
}

TEST(WindowHostTest, ReleasingRenderContextBeforeMakeCurrentDoesNotBindRenderThread)
{
	auto bundle = sparkle_test::createWindowHostBundle();

	bundle.windowHost->releaseRenderContext();

	EXPECT_FALSE(bundle.windowHost->isRenderThread());

	std::jthread renderThread([&](){
		EXPECT_TRUE(bundle.windowHost->makeCurrent());
		EXPECT_TRUE(bundle.windowHost->isRenderThread());
		bundle.windowHost->releaseRenderContext();
	});
	renderThread.join();

	EXPECT_EQ(bundle.gpuPlatformRuntime->createRenderContextCount, 1);
	EXPECT_EQ(bundle.gpuPlatformRuntime->contextStats->makeCurrentCount, 1);
}

TEST(WindowHostTest, ReleasingRenderContextFromNonRenderThreadAfterMakeCurrentThrows)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	std::atomic<bool> contextCreated = false;
	std::atomic<bool> allowRelease = false;

	std::jthread renderThread([&]()
	{
		EXPECT_TRUE(bundle.windowHost->makeCurrent());
		contextCreated.store(true);

		while (allowRelease.load() == false)
		{
			std::this_thread::yield();
		}

		bundle.windowHost->releaseRenderContext();
	});

	while (contextCreated.load() == false)
	{
		std::this_thread::yield();
	}

	EXPECT_THROW(bundle.windowHost->releaseRenderContext(), std::runtime_error);

	allowRelease.store(true);
	renderThread.join();
}

TEST(WindowHostTest, NotifyFrameResizedBeforeMakeCurrentIsIgnored)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	const spk::Rect2D newRect(0, 0, 640, 480);

	EXPECT_NO_THROW(bundle.windowHost->notifyFrameResized(newRect));
	EXPECT_EQ(bundle.gpuPlatformRuntime->createdContext, nullptr);
}

TEST(WindowHostTest, NotifyFrameResizedAfterMakeCurrentForwardsToRenderContext)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	const spk::Rect2D newRect(0, 0, 640, 480);

	std::jthread renderThread([&]()
	{
		ASSERT_TRUE(bundle.windowHost->makeCurrent());
		bundle.windowHost->notifyFrameResized(newRect);
		bundle.windowHost->releaseRenderContext();
	});
	renderThread.join();

	ASSERT_NE(bundle.gpuPlatformRuntime->contextStats, nullptr);
	EXPECT_EQ(bundle.gpuPlatformRuntime->contextStats->notifyResizeCount, 1);
	EXPECT_EQ(bundle.gpuPlatformRuntime->contextStats->lastResizeRect, newRect);
}

TEST(WindowHostTest, DestructorInvalidatesLiveRenderAndFrameState)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	std::shared_ptr<sparkle_test::TestFrameStats> frameStats = bundle.platformRuntime->frameStats;
	std::shared_ptr<spk::SurfaceState> surfaceState = bundle.platformRuntime->createdFrame->surfaceState();

	ASSERT_TRUE(bundle.windowHost->makeCurrent());
	std::shared_ptr<sparkle_test::TestRenderContextStats> contextStats = bundle.gpuPlatformRuntime->contextStats;

	bundle.windowHost.reset();

	ASSERT_NE(surfaceState, nullptr);
	EXPECT_FALSE(surfaceState->isValid());
	ASSERT_NE(contextStats, nullptr);
	EXPECT_EQ(contextStats->invalidateCount, 1);
	EXPECT_TRUE(contextStats->destroyed);
	ASSERT_NE(frameStats, nullptr);
	EXPECT_TRUE(frameStats->destroyed);
}
