#include <gtest/gtest.h>

#include "window_test_utils.hpp"

namespace
{
	class NullSurfaceStateRenderContext : public spk::RenderContext
	{
	public:
		NullSurfaceStateRenderContext() :
			spk::RenderContext(nullptr)
		{
		}

		void makeCurrent() override
		{
		}

		void present() override
		{
		}

		void setVSync(bool) override
		{
		}

		void notifyResize(const spk::Rect2D&) override
		{
		}
	};
}

TEST(RenderContextTest, TestRenderContextTracksAllOperations)
{
	sparkle_test::TestRenderContext context(std::make_shared<spk::SurfaceState>());
	const spk::Rect2D resizedRect(3, 4, 1280, 720);

	context.makeCurrent();
	context.present();
	context.setVSync(true);
	context.notifyResize(resizedRect);

	EXPECT_EQ(context.makeCurrentCount, 1);
	EXPECT_EQ(context.presentCount, 1);
	EXPECT_EQ(context.setVSyncCount, 1);
	EXPECT_TRUE(context.lastVSync);
	EXPECT_EQ(context.notifyResizeCount, 1);
	EXPECT_EQ(context.lastResizeRect, resizedRect);
	ASSERT_EQ(context.resizeHistory.size(), 1u);
	EXPECT_EQ(context.resizeHistory[0], resizedRect);
}

TEST(RenderContextTest, InvalidateMarksContextAsInvalid)
{
	sparkle_test::TestRenderContext context(std::make_shared<spk::SurfaceState>());

	EXPECT_TRUE(context.isValid());

	context.invalidate();

	EXPECT_FALSE(context.isValid());
	EXPECT_EQ(context.invalidateCount, 1);
}

TEST(RenderContextTest, ConstructingRenderContextWithoutSurfaceStateIsInvalid)
{
	NullSurfaceStateRenderContext ctx;
	EXPECT_FALSE(ctx.isValid());
}

TEST(RenderContextTest, ConstructingWithSurfaceStateSharesValidityWithTheSurface)
{
	auto surfaceState = std::make_shared<spk::SurfaceState>();
	sparkle_test::TestRenderContext context(surfaceState);

	EXPECT_EQ(context.surfaceState(), surfaceState);
	EXPECT_TRUE(context.isValid());

	surfaceState->invalidate();

	EXPECT_FALSE(context.isValid());
}

TEST(GPUPlatformTest, CreateRenderContextReceivesFrameReference)
{
	sparkle_test::TestGPUPlatformRuntime gpuPlatformRuntime;
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");

	std::unique_ptr<spk::RenderContext> context = gpuPlatformRuntime.createRenderContext(frame);

	ASSERT_NE(context, nullptr);
	ASSERT_NE(gpuPlatformRuntime.createdContext, nullptr);
	EXPECT_EQ(gpuPlatformRuntime.createRenderContextCount, 1);
	EXPECT_EQ(gpuPlatformRuntime.lastFrame, &frame);
	EXPECT_TRUE(context->surfaceState() != nullptr);
}

TEST(GPUPlatformTest, RuntimeCanBeConfiguredToReturnNullContext)
{
	sparkle_test::TestGPUPlatformRuntime gpuPlatformRuntime;
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	gpuPlatformRuntime.returnNullContext = true;

	std::unique_ptr<spk::RenderContext> context = gpuPlatformRuntime.createRenderContext(frame);

	EXPECT_EQ(context, nullptr);
	EXPECT_EQ(gpuPlatformRuntime.createRenderContextCount, 1);
	EXPECT_EQ(gpuPlatformRuntime.lastFrame, &frame);
	EXPECT_EQ(gpuPlatformRuntime.createdContext, nullptr);
}

TEST(GPUPlatformTest, WaitUntilWorkDoneIsTrackedByTestRuntime)
{
	sparkle_test::TestGPUPlatformRuntime gpuPlatformRuntime;

	gpuPlatformRuntime.waitUntilWorkDone();
	gpuPlatformRuntime.waitUntilWorkDone();

	EXPECT_EQ(gpuPlatformRuntime.waitUntilWorkDoneCount, 2);
}
