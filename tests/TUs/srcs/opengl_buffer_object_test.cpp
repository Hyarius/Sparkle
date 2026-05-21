#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLBufferObjectTest, SynchronizesBinaryFieldToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::array<std::uint32_t, 4> values = {10, 20, 30, 40};
	spk::OpenGL::BufferObject buffer(
		spk::OpenGL::BufferObject::Target::Array,
		spk::OpenGL::BufferObject::Usage::DynamicDraw,
		sizeof(values));

	spk::BinaryField entries = buffer.field().addArray("entries", 0, values.size(), sizeof(std::uint32_t));
	for (std::size_t i = 0; i < values.size(); ++i)
	{
		entries[i] = values[i];
	}

	buffer.synchronize();
	buffer.activate();

	std::array<std::uint32_t, 4> gpuValues = {};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpuValues), gpuValues.data());

	EXPECT_EQ(gpuValues, values);
	EXPECT_FALSE(buffer.needsSynchronization());
	EXPECT_TRUE(buffer.isAllocated());
}

TEST(OpenGLBufferObjectTest, EditAppendResizeAndUsageRequestSynchronization)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::BufferObject buffer(
		spk::OpenGL::BufferObject::Target::Array,
		spk::OpenGL::BufferObject::Usage::DynamicDraw,
		0);

	std::array<std::uint8_t, 2> first = {1, 2};
	std::array<std::uint8_t, 2> second = {3, 4};
	buffer.append(first.data(), first.size());
	buffer.append(second.data(), second.size());

	EXPECT_EQ(buffer.size(), 4u);
	EXPECT_TRUE(buffer.needsSynchronization());

	std::array<std::uint8_t, 2> patch = {9, 8};
	buffer.edit(patch.data(), patch.size(), 1);
	buffer.activate();

	std::array<std::uint8_t, 4> gpuValues = {};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpuValues), gpuValues.data());
	EXPECT_EQ(gpuValues, (std::array<std::uint8_t, 4>{1, 9, 8, 4}));

	buffer.setUsage(spk::OpenGL::BufferObject::Usage::StaticDraw);
	EXPECT_TRUE(buffer.needsSynchronization());

	buffer.resize(2);
	EXPECT_EQ(buffer.size(), 2u);
	EXPECT_EQ(buffer.field().size(), 2u);
}

#endif
