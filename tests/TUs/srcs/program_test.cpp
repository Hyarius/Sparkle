#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(ProgramTest, CompilesLinksBindsAndUnbinds)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::shared_ptr<spk::Program> program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	program->bind();

	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	EXPECT_EQ(static_cast<GLuint>(currentProgram), program->id());
	EXPECT_TRUE(program->isLinked());

	program->unbind();
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	EXPECT_EQ(currentProgram, 0);
}

TEST(ProgramTest, RelinksAfterSourceUpdateAndRejectsInvalidSource)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::shared_ptr<spk::Program> program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	program->bind();
	program->setSources(
		sparkle_test::makeSolidProgram(0.0f, 1.0f, 0.0f)->vertexShaderSource(),
		sparkle_test::makeSolidProgram(0.0f, 1.0f, 0.0f)->fragmentShaderSource());
	EXPECT_TRUE(program->needsSynchronization());
	program->bind();
	EXPECT_FALSE(program->needsSynchronization());
	EXPECT_TRUE(program->isLinked());

	program->setSources("not valid GLSL", "not valid GLSL");
	EXPECT_THROW(program->synchronize(), std::runtime_error);
	EXPECT_TRUE(program->needsSynchronization());
}

#endif
