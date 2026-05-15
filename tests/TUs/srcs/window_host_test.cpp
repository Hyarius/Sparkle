#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <thread>

#include "window_test_utils.hpp"

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

	std::shared_ptr<spk::ISurfaceState> surfaceState = bundle.platformRuntime->createdFrame->surfaceState();
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

TEST(WindowHostTest, DestructorInvalidatesLiveRenderAndFrameState)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	std::shared_ptr<sparkle_test::TestFrameStats> frameStats = bundle.platformRuntime->frameStats;
	std::shared_ptr<spk::ISurfaceState> surfaceState = bundle.platformRuntime->createdFrame->surfaceState();

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
