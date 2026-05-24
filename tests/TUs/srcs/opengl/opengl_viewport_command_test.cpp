#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"


#include "opengl/spk_opengl_viewport.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"

using Viewport = spk::Viewport;

TEST(ViewportCommandTest, AppliesRectToOpenGLViewport)
{
	sparkle_test::OpenGLTestContext context;
	spk::RenderContext& renderContext = context.renderContext();

	Viewport viewport(spk::Rect2D(1, 2, 11, 12));
	spk::ViewportCommand command(viewport);
	command.execute(renderContext);

	GLint vp[4] = {};
	glGetIntegerv(GL_VIEWPORT, vp);
	EXPECT_EQ(vp[0], 1);
	EXPECT_EQ(vp[1], 18);
	EXPECT_EQ(vp[2], 11);
	EXPECT_EQ(vp[3], 12);
}

