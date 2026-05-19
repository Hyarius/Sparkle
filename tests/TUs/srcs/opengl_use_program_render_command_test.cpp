#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLUseProgramRenderCommandTest, BindsAndUnbindsProgram)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	auto program = sparkle_test::makeSolidProgram(0.0f, 1.0f, 0.0f);
	spk::OpenGL::UseProgramRenderCommand bindCommand(program);
	bindCommand.execute(renderContext);

	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	EXPECT_EQ(static_cast<GLuint>(currentProgram), program->id());

	spk::OpenGL::UseProgramRenderCommand unbindCommand;
	unbindCommand.execute(renderContext);
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	EXPECT_EQ(currentProgram, 0);
}

#endif
