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

TEST(IRenderContextBackendTest, CreateRenderContextReceivesFrameReference)
{
	sparkle_test::TestRenderContextBackend backend;
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");

	std::unique_ptr<spk::IRenderContext> context = backend.createRenderContext(frame);

	ASSERT_NE(context, nullptr);
	ASSERT_NE(backend.createdContext, nullptr);
	EXPECT_EQ(backend.createRenderContextCount, 1);
	EXPECT_EQ(backend.lastFrame, &frame);
}

TEST(IRenderContextBackendTest, BackendCanBeConfiguredToReturnNullContext)
{
	sparkle_test::TestRenderContextBackend backend;
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	backend.returnNullContext = true;

	std::unique_ptr<spk::IRenderContext> context = backend.createRenderContext(frame);

	EXPECT_EQ(context, nullptr);
	EXPECT_EQ(backend.createRenderContextCount, 1);
	EXPECT_EQ(backend.lastFrame, &frame);
	EXPECT_EQ(backend.createdContext, nullptr);
}
