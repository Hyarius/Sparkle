#include <array>
#include <cstdint>
#include <stdexcept>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

using namespace spk;

TEST(OpenGLIndexBufferObjectTest, StoresDrawMetadataAndSynchronizesData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::array<std::uint16_t, 3> indices = {0, 1, 2};
	IndexBufferObject indexBuffer(BufferObject::Usage::StaticDraw, sizeof(indices));
	indexBuffer.setElementType(GL_UNSIGNED_SHORT);
	indexBuffer.edit(indices.data(), sizeof(indices));
	indexBuffer.activate(context.renderContext());

	std::array<std::uint16_t, 3> gpuIndices = {};
	glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(gpuIndices), gpuIndices.data());

	EXPECT_EQ(indexBuffer.target(), BufferObject::Target::ElementArray);
	EXPECT_EQ(indexBuffer.elementType(), GL_UNSIGNED_SHORT);
	EXPECT_EQ(indexBuffer.count(), indices.size());
	EXPECT_EQ(gpuIndices, indices);
}

TEST(OpenGLIndexBufferObjectTest, SupportsUnsignedByteElementType)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	IndexBufferObject indexBuffer(BufferObject::Usage::StaticDraw, 4);
	indexBuffer.setElementType(GL_UNSIGNED_BYTE);

	EXPECT_EQ(indexBuffer.elementType(), GL_UNSIGNED_BYTE);
	EXPECT_EQ(indexBuffer.count(), 4u);
}

TEST(OpenGLIndexBufferObjectTest, RejectsUnsupportedElementType)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	IndexBufferObject indexBuffer(BufferObject::Usage::StaticDraw, 4);
	EXPECT_THROW(indexBuffer.setElementType(GL_FLOAT), std::runtime_error);
	EXPECT_EQ(indexBuffer.elementType(), static_cast<GLenum>(GL_UNSIGNED_INT));
}

