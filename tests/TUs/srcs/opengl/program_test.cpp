#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "opengl_wrapper_test_utils.hpp"

using BufferObject = spk::BufferObject;
using IndexBufferObject = spk::IndexBufferObject;
using Primitive = spk::Primitive;

TEST(ProgramTest, CompilesLinksActivatesAndDeactivates)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::shared_ptr<spk::Program> program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	program->activate();

	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	EXPECT_EQ(static_cast<GLuint>(currentProgram), program->id());
	EXPECT_TRUE(program->isLinked());

	program->deactivate();
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
	EXPECT_EQ(currentProgram, 0);
}

TEST(ProgramTest, RelinksAfterSourceUpdateAndRejectsInvalidSource)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::shared_ptr<spk::Program> program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	program->activate();
	program->setSources(
		sparkle_test::makeSolidProgram(0.0f, 1.0f, 0.0f)->vertexShaderSource(),
		sparkle_test::makeSolidProgram(0.0f, 1.0f, 0.0f)->fragmentShaderSource());
	EXPECT_TRUE(program->needsSynchronization());
	program->activate();
	EXPECT_FALSE(program->needsSynchronization());
	EXPECT_TRUE(program->isLinked());

	program->setSources("not valid GLSL", "not valid GLSL");
	EXPECT_THROW(program->synchronize(), std::runtime_error);
	EXPECT_TRUE(program->needsSynchronization());
}

TEST(ProgramTest, RenderHelpersIssueDrawCalls)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const auto program = sparkle_test::makeColorProgram();
	const auto vertexArray = sparkle_test::makeTriangleVAO(
		sparkle_test::fullScreenTriangle({1.0f, 0.0f, 0.0f}));
	const std::array<std::uint32_t, 3> indexes = {0, 1, 2};
	auto indexBuffer = std::make_shared<IndexBufferObject>(
		BufferObject::Usage::StaticDraw,
		sizeof(indexes));
	indexBuffer->setElementType(GL_UNSIGNED_INT);
	indexBuffer->setCount(indexes.size());
	indexBuffer->edit(indexes.data(), sizeof(indexes));
	vertexArray->setIndexBuffer(indexBuffer);
	vertexArray->activate();

	EXPECT_NO_THROW(program->renderRaw(Primitive::Triangles, 0, 3));
	EXPECT_NO_THROW(program->render(Primitive::Triangles, 0, 3));
}

