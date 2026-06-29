#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

using BufferObject = spk::BufferObject;
using UniformBufferObject = spk::UniformBufferObject;

namespace
{
	struct ViewportBlock
	{
		float projection[16] = {};
	};

	static_assert(std::is_trivially_copyable_v<ViewportBlock>);
	static_assert(sizeof(ViewportBlock) == 64);
}

TEST(OpenGLUniformBufferObjectTest, CastOverlaysBufferAndTracksWrites)
{
	UniformBufferObject uniformBuffer(0, BufferObject::Usage::DynamicDraw, sizeof(ViewportBlock));

	const std::uint64_t before = uniformBuffer.contentVersion();

	ViewportBlock &block = uniformBuffer.cast<ViewportBlock>();
	EXPECT_EQ(static_cast<void *>(&block), static_cast<void *>(uniformBuffer.data()));
	EXPECT_GT(uniformBuffer.contentVersion(), before);

	block.projection[0] = 3.5f;
	block.projection[15] = 9.0f;

	const ViewportBlock &readOnly =
		static_cast<const UniformBufferObject &>(uniformBuffer).cast<ViewportBlock>();
	EXPECT_FLOAT_EQ(readOnly.projection[0], 3.5f);
	EXPECT_FLOAT_EQ(readOnly.projection[15], 9.0f);
}

TEST(OpenGLUniformBufferObjectTest, CastThrowsWhenTypeSizeDoesNotMatchBuffer)
{
	UniformBufferObject uniformBuffer(0, BufferObject::Usage::DynamicDraw, sizeof(ViewportBlock) - 4);

	EXPECT_THROW({ [[maybe_unused]] ViewportBlock &block = uniformBuffer.cast<ViewportBlock>(); }, std::runtime_error);
	EXPECT_THROW(
		{
			[[maybe_unused]] const ViewportBlock &block =
				static_cast<const UniformBufferObject &>(uniformBuffer).cast<ViewportBlock>();
		},
		std::runtime_error);
}

TEST(OpenGLUniformBufferObjectTest, BindsConfiguredBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	UniformBufferObject uniformBuffer(3, BufferObject::Usage::DynamicDraw, 16);
	uniformBuffer.activate(context.renderContext());

	GLint boundBuffer = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 3, &boundBuffer);
	EXPECT_EQ(static_cast<GLuint>(boundBuffer), uniformBuffer.gpu(context.renderContext()).id());
	EXPECT_EQ(uniformBuffer.bindingPoint(), 3u);
}

TEST(OpenGLUniformBufferObjectTest, ValidatesProgramThatExposesBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Program program(
		"#version 330 core\n"
		"layout(std140) uniform ViewportData\n"
		"{\n"
		"	mat4 projection;\n"
		"};\n"
		"void main()\n"
		"{\n"
		"	gl_Position = projection * vec4(0.0, 0.0, 0.0, 1.0);\n"
		"}\n",
		"#version 330 core\n"
		"out vec4 outColor;\n"
		"void main()\n"
		"{\n"
		"	outColor = vec4(1.0);\n"
		"}\n");
	UniformBufferObject uniformBuffer(
		0,
		BufferObject::Usage::DynamicDraw,
		sizeof(float) * 16);

	EXPECT_NO_THROW(uniformBuffer.validateFor(program));

	const GLuint blockIndex = glGetUniformBlockIndex(program.id(), "ViewportData");
	ASSERT_NE(blockIndex, GL_INVALID_INDEX);
	GLint blockBindingPoint = -1;
	glGetActiveUniformBlockiv(program.id(), blockIndex, GL_UNIFORM_BLOCK_BINDING, &blockBindingPoint);
	EXPECT_EQ(blockBindingPoint, 0);

}

TEST(OpenGLUniformBufferObjectTest, ValidationThrowsWhenProgramDoesNotExposeBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::shared_ptr<spk::Program> program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	UniformBufferObject uniformBuffer(
		0,
		BufferObject::Usage::DynamicDraw,
		sizeof(float) * 16);

	EXPECT_THROW(uniformBuffer.validateFor(*program), std::runtime_error);
}

TEST(OpenGLUniformBufferObjectTest, ValidationThrowsWhenProgramBlockUsesDifferentBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Program program(
		"#version 330 core\n"
		"layout(std140) uniform AnyBlock\n"
		"{\n"
		"	mat4 transform;\n"
		"};\n"
		"void main()\n"
		"{\n"
		"	gl_Position = transform * vec4(0.0, 0.0, 0.0, 1.0);\n"
		"}\n",
		"#version 330 core\n"
		"out vec4 outColor;\n"
		"void main()\n"
		"{\n"
		"	outColor = vec4(1.0);\n"
		"}\n");
	UniformBufferObject uniformBuffer(
		3,
		BufferObject::Usage::DynamicDraw,
		sizeof(float) * 16);

	EXPECT_THROW(uniformBuffer.validateFor(program), std::runtime_error);
}

TEST(OpenGLUniformBufferObjectTest, ValidationObservesProgramRelink)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Program program(
		"#version 330 core\n"
		"layout(std140) uniform ViewportData\n"
		"{\n"
		"	mat4 projection;\n"
		"};\n"
		"void main()\n"
		"{\n"
		"	gl_Position = projection * vec4(0.0, 0.0, 0.0, 1.0);\n"
		"}\n",
		"#version 330 core\n"
		"out vec4 outColor;\n"
		"void main()\n"
		"{\n"
		"	outColor = vec4(1.0);\n"
		"}\n");
	UniformBufferObject uniformBuffer(
		0,
		BufferObject::Usage::DynamicDraw,
		sizeof(float) * 16);

	EXPECT_NO_THROW(uniformBuffer.validateFor(program));

	const std::shared_ptr<spk::Program> withoutBlock = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	program.setSources(withoutBlock->vertexShaderSource(), withoutBlock->fragmentShaderSource());
	EXPECT_THROW(uniformBuffer.validateFor(program), std::runtime_error);
}

