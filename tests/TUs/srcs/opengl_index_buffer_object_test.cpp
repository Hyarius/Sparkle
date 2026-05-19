#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLIndexBufferObjectTest, StoresDrawMetadataAndSynchronizesData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::array<std::uint16_t, 3> indices = {0, 1, 2};
	spk::OpenGL::IndexBufferObject indexBuffer(spk::OpenGL::BufferObject::Usage::StaticDraw, sizeof(indices));
	indexBuffer.setElementType(GL_UNSIGNED_SHORT);
	indexBuffer.setCount(indices.size());
	indexBuffer.edit(indices.data(), sizeof(indices));
	indexBuffer.bind();

	std::array<std::uint16_t, 3> gpuIndices = {};
	glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(gpuIndices), gpuIndices.data());

	EXPECT_EQ(indexBuffer.target(), spk::OpenGL::BufferObject::Target::ElementArray);
	EXPECT_EQ(indexBuffer.elementType(), GL_UNSIGNED_SHORT);
	EXPECT_EQ(indexBuffer.count(), indices.size());
	EXPECT_EQ(gpuIndices, indices);
}

#endif
