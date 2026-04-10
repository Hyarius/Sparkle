#include <gtest/gtest.h>

#include "window_test_utils.hpp"

TEST(IRenderContextTest, TestRenderContextTracksAllOperations)
{
	sparkle_test::TestRenderContext context;
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

TEST(IGPUPlatformRuntimeTest, CreateRenderContextReceivesFrameReference)
{
	sparkle_test::TestGPUPlatformRuntime gpuPlatformRuntime;
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");

	std::unique_ptr<spk::IRenderContext> context = gpuPlatformRuntime.createRenderContext(frame);

	ASSERT_NE(context, nullptr);
	ASSERT_NE(gpuPlatformRuntime.createdContext, nullptr);
	EXPECT_EQ(gpuPlatformRuntime.createRenderContextCount, 1);
	EXPECT_EQ(gpuPlatformRuntime.lastFrame, &frame);
}

TEST(IGPUPlatformRuntimeTest, RuntimeCanBeConfiguredToReturnNullContext)
{
	sparkle_test::TestGPUPlatformRuntime gpuPlatformRuntime;
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	gpuPlatformRuntime.returnNullContext = true;

	std::unique_ptr<spk::IRenderContext> context = gpuPlatformRuntime.createRenderContext(frame);

	EXPECT_EQ(context, nullptr);
	EXPECT_EQ(gpuPlatformRuntime.createRenderContextCount, 1);
	EXPECT_EQ(gpuPlatformRuntime.lastFrame, &frame);
	EXPECT_EQ(gpuPlatformRuntime.createdContext, nullptr);
}
