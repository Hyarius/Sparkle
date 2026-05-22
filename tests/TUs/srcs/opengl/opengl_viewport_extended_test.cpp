#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl/spk_opengl_viewport.hpp"

TEST(OpenGLViewportTest, TwoArgConstructorSetsGeometryAndScissorIndependently)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const spk::Rect2D geometry(10, 20, 100, 80);
	const spk::Rect2D scissor(15, 25, 60, 50);

	spk::OpenGL::Viewport viewport(geometry, scissor);

	EXPECT_EQ(viewport.geometry(), geometry);
	EXPECT_EQ(viewport.scissor(), scissor);
}

TEST(OpenGLViewportTest, ActivateThrowsWhenWindowWidthIsZero)
{
	spk::WinAPI::PlatformRuntime platformRuntime;
	spk::OpenGL::Runtime gpuRuntime;
	auto frame = platformRuntime.createFrame(spk::Rect2D(0, 0, 32, 32), "ZeroWidth");
	auto renderContext = gpuRuntime.createRenderContext(*frame);
	renderContext->makeCurrent();
	renderContext->notifyResize(spk::Rect2D(0, 0, 0, 32));

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 10, 10));

	EXPECT_THROW(
		viewport.activate(*renderContext->surfaceState()),
		std::runtime_error);

	frame->validateClosure();
	platformRuntime.pollEvents();
}

TEST(OpenGLViewportTest, ActivateThrowsWhenWindowHeightIsZero)
{
	spk::WinAPI::PlatformRuntime platformRuntime;
	spk::OpenGL::Runtime gpuRuntime;
	auto frame = platformRuntime.createFrame(spk::Rect2D(0, 0, 32, 32), "ZeroHeight");
	auto renderContext = gpuRuntime.createRenderContext(*frame);
	renderContext->makeCurrent();
	renderContext->notifyResize(spk::Rect2D(0, 0, 32, 0));

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 10, 10));

	EXPECT_THROW(
		viewport.activate(*renderContext->surfaceState()),
		std::runtime_error);

	frame->validateClosure();
	platformRuntime.pollEvents();
}

TEST(OpenGLViewportTest, ActivateSetsActiveViewportPointer)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 32, 32));
	viewport.activate(*renderContext.surfaceState());

	EXPECT_EQ(spk::Viewport::activeViewport(), &viewport);
}

TEST(OpenGLViewportTest, ConvertScreenToOpenGLVector2IntMapsOriginCorrectly)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	const float savedMaxLayer = spk::Viewport::maxLayer();
	spk::Viewport::setMaxLayer(1000.0f);

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 100, 100));
	viewport.activate(*renderContext.surfaceState());

	const spk::Vector2 result = spk::Viewport::convertScreenToOpenGL(spk::Vector2Int{0, 0});
	EXPECT_FLOAT_EQ(result.x, -1.0f);
	EXPECT_FLOAT_EQ(result.y, 1.0f);

	spk::Viewport::setMaxLayer(savedMaxLayer);
}

TEST(OpenGLViewportTest, ConvertScreenToOpenGLIntXYOverloadMatchesVector2Int)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	const float savedMaxLayer = spk::Viewport::maxLayer();
	spk::Viewport::setMaxLayer(1000.0f);

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 100, 100));
	viewport.activate(*renderContext.surfaceState());

	const spk::Vector2 vecResult = spk::Viewport::convertScreenToOpenGL(spk::Vector2Int{50, 50});
	const spk::Vector2 xyResult = spk::Viewport::convertScreenToOpenGL(50, 50);
	EXPECT_FLOAT_EQ(vecResult.x, xyResult.x);
	EXPECT_FLOAT_EQ(vecResult.y, xyResult.y);

	spk::Viewport::setMaxLayer(savedMaxLayer);
}

TEST(OpenGLViewportTest, ConvertScreenToOpenGLWithLayerReturnsVector3)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	const float savedMaxLayer = spk::Viewport::maxLayer();
	spk::Viewport::setMaxLayer(1000.0f);

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 100, 100));
	viewport.activate(*renderContext.surfaceState());

	const spk::Vector3 result3 = spk::Viewport::convertScreenToOpenGL(spk::Vector2Int{0, 0}, 0.0f);
	EXPECT_FLOAT_EQ(result3.z, 0.0f);

	const spk::Vector3 result3xy = spk::Viewport::convertScreenToOpenGL(0, 0, 0.0f);
	EXPECT_FLOAT_EQ(result3.x, result3xy.x);
	EXPECT_FLOAT_EQ(result3.y, result3xy.y);
	EXPECT_FLOAT_EQ(result3.z, result3xy.z);

	spk::Viewport::setMaxLayer(savedMaxLayer);
}

#endif
