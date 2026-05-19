#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLBindVertexArrayCommandTest, BindsAndUnbindsVertexArray)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	auto vertexArray = std::make_shared<spk::OpenGL::VertexArrayObject>();
	spk::OpenGL::BindVertexArrayCommand bindCommand(vertexArray);
	bindCommand.execute(renderContext);

	GLint currentVertexArray = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVertexArray);
	EXPECT_EQ(static_cast<GLuint>(currentVertexArray), vertexArray->id());

	spk::OpenGL::BindVertexArrayCommand unbindCommand;
	unbindCommand.execute(renderContext);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVertexArray);
	EXPECT_EQ(currentVertexArray, 0);
}

#endif
