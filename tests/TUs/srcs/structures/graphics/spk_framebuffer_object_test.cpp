#include <gtest/gtest.h>

#include <cstdint>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"

TEST(FrameBufferObjectTest, DefaultsToEmptyWithDepth)
{
	spk::FrameBufferObject fbo;

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(0, 0));
	EXPECT_TRUE(fbo.hasDepth());
}

TEST(FrameBufferObjectTest, ConstructsWithSizeAndConfiguresViewport)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(64, 48));

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(64, 48));
	EXPECT_EQ(fbo.viewport().geometry(), spk::Rect2D(0, 0, 64, 48));
	EXPECT_EQ(fbo.viewport().scissor(), spk::Rect2D(0, 0, 64, 48));
	EXPECT_EQ(fbo.colorAttachment().size(), spk::Vector2UInt(64, 48));
	EXPECT_EQ(fbo.surfaceState().rect(), spk::Rect2D(0, 0, 64, 48));
}

TEST(FrameBufferObjectTest, CanBeConstructedWithoutDepth)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(16, 16), false);

	EXPECT_FALSE(fbo.hasDepth());
}

TEST(FrameBufferObjectTest, ResizeUpdatesStateAndBumpsVersion)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	const std::uint64_t before = fbo.version();

	fbo.resize(spk::Vector2UInt(16, 8));

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(16, 8));
	EXPECT_EQ(fbo.viewport().geometry(), spk::Rect2D(0, 0, 16, 8));
	EXPECT_EQ(fbo.colorAttachment().size(), spk::Vector2UInt(16, 8));
	EXPECT_GT(fbo.version(), before);
}

TEST(FrameBufferObjectTest, ResizeToSameSizeIsANoOp)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	const std::uint64_t before = fbo.version();

	fbo.resize(spk::Vector2UInt(32, 32));

	EXPECT_EQ(fbo.version(), before);
}

TEST(FrameBufferObjectTest, BuildsACompleteGpuFramebuffer)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 64, 64));

	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	spk::OpenGL::FrameBufferObject &gpu = fbo.gpu(context.renderContext());

	EXPECT_NE(gpu.id(), 0u);
	EXPECT_TRUE(gpu.isComplete());
	EXPECT_NE(fbo.colorAttachment().gpu(context.renderContext()).id(), 0u);
}

TEST(FrameBufferObjectTest, RebuildsACompleteGpuFramebufferAfterResize)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 64, 64));

	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	EXPECT_TRUE(fbo.gpu(context.renderContext()).isComplete());

	fbo.resize(spk::Vector2UInt(48, 24));

	EXPECT_TRUE(fbo.gpu(context.renderContext()).isComplete());
}

TEST(FrameBufferObjectTest, HasGpuOnlyAfterResolution)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 64, 64));

	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	EXPECT_FALSE(fbo.hasGpu(context.renderContext()));

	(void)fbo.gpu(context.renderContext());
	EXPECT_TRUE(fbo.hasGpu(context.renderContext()));
}
