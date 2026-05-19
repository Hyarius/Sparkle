#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLViewportCommandTest, AppliesRectToOpenGLViewport)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	spk::OpenGL::ViewportCommand command(spk::Rect2D(1, 2, 11, 12));
	command.execute(renderContext);

	GLint viewport[4] = {};
	glGetIntegerv(GL_VIEWPORT, viewport);
	EXPECT_EQ(viewport[0], 1);
	EXPECT_EQ(viewport[1], 2);
	EXPECT_EQ(viewport[2], 11);
	EXPECT_EQ(viewport[3], 12);
}

#endif
