#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

TEST(OpenGLBufferObjectTest, SynchronizesBinaryFieldToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::array<std::uint32_t, 4> values = {10, 20, 30, 40};
	spk::BufferObject buffer(spk::BufferObject::Target::Array, spk::BufferObject::Usage::DynamicDraw, sizeof(values));

	spk::BinaryField entries = buffer.field().addArray("entries", 0, values.size(), sizeof(std::uint32_t));
	for (std::size_t i = 0; i < values.size(); ++i)
	{
		entries[i] = values[i];
	}

	buffer.synchronize();
	buffer.activate(context.renderContext());

	std::array<std::uint32_t, 4> gpuValues = {};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpuValues), gpuValues.data());

	EXPECT_EQ(gpuValues, values);
	EXPECT_FALSE(buffer.needsSynchronization());
	EXPECT_TRUE(buffer.hasGpu(context.renderContext()));
}

TEST(OpenGLBufferObjectTest, EditAppendAndResizeRequestSynchronization)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::BufferObject buffer(spk::BufferObject::Target::Array, spk::BufferObject::Usage::DynamicDraw, 0);

	std::array<std::uint8_t, 2> first = {1, 2};
	std::array<std::uint8_t, 2> second = {3, 4};
	buffer.append(first.data(), first.size());
	buffer.append(second.data(), second.size());

	EXPECT_EQ(buffer.size(), 4u);
	EXPECT_TRUE(buffer.needsSynchronization());

	std::array<std::uint8_t, 2> patch = {9, 8};
	buffer.edit(patch.data(), patch.size(), 1);
	buffer.activate(context.renderContext());

	std::array<std::uint8_t, 4> gpuValues = {};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpuValues), gpuValues.data());
	EXPECT_EQ(gpuValues, (std::array<std::uint8_t, 4>{1, 9, 8, 4}));

	buffer.resize(2);
	EXPECT_EQ(buffer.size(), 2u);
	EXPECT_EQ(buffer.field().size(), 2u);
}

TEST(OpenGLBufferObjectTest, AccessorsAndViewsExposeCpuBuffer)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::BufferObject buffer(spk::BufferObject::Target::Array, spk::BufferObject::Usage::StreamDraw, 4);

	std::uint8_t* data = buffer.data();
	data[0] = 4;
	data[1] = 3;
	data[2] = 2;
	data[3] = 1;

	std::span<std::uint8_t> bytes = buffer.bytes();
	bytes[2] = 9;

	const spk::BufferObject& constBuffer = buffer;
	std::span<const std::uint8_t> constBytes = constBuffer.bytes();

	EXPECT_EQ(buffer.target(), spk::BufferObject::Target::Array);
	EXPECT_EQ(buffer.usage(), spk::BufferObject::Usage::StreamDraw);
	EXPECT_EQ(constBuffer.data()[0], 4u);
	EXPECT_EQ(constBytes[2], 9u);
	EXPECT_EQ(constBuffer.field().size(), 4u);
	EXPECT_TRUE(buffer.needsSynchronization());
}

TEST(OpenGLBufferObjectTest, ClearResizesBufferToZero)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::BufferObject buffer(spk::BufferObject::Target::Array, spk::BufferObject::Usage::DynamicDraw, 4);
	buffer.synchronize();
	ASSERT_FALSE(buffer.needsSynchronization());

	buffer.clear();
	EXPECT_EQ(buffer.size(), 0u);
	EXPECT_EQ(buffer.field().size(), 0u);
}

TEST(OpenGLBufferObjectTest, EditAndAppendValidateInputs)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::BufferObject buffer(spk::BufferObject::Target::Array, spk::BufferObject::Usage::DynamicDraw, 4);

	EXPECT_THROW(buffer.edit(nullptr, 1), std::runtime_error);
	EXPECT_THROW(buffer.edit(nullptr, 0, 5), std::runtime_error);
	EXPECT_THROW(buffer.append(nullptr, 1), std::runtime_error);

	EXPECT_NO_THROW(buffer.edit(nullptr, 0));
	EXPECT_NO_THROW(buffer.append(nullptr, 0));
}

TEST(OpenGLBufferObjectTest, ActivationHelpersBindBaseAndRange)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::array<std::uint8_t, 8> values = {0, 1, 2, 3, 4, 5, 6, 7};
	spk::BufferObject buffer(spk::BufferObject::Target::Uniform, spk::BufferObject::Usage::DynamicDraw, values.size());
	buffer.edit(values.data(), values.size());

	buffer.activateBase(context.renderContext(), 0);
	GLint boundBase = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 0, &boundBase);
	EXPECT_EQ(static_cast<GLuint>(boundBase), buffer.gpu(context.renderContext()).id());

	buffer.activateRange(context.renderContext(), 1, 0, static_cast<GLsizeiptr>(values.size()));
	GLint boundRange = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 1, &boundRange);
	EXPECT_EQ(static_cast<GLuint>(boundRange), buffer.gpu(context.renderContext()).id());

	buffer.deactivate();
	GLint boundUniform = 1;
	glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &boundUniform);
	EXPECT_EQ(boundUniform, 0);
}
