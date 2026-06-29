#include <array>
#include <cstdint>
#include <span>
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

TEST(OpenGLIndexBufferObjectTest, CastSpansBufferUsingDefaultIndexType)
{
	IndexBufferObject indexBuffer(BufferObject::Usage::StaticDraw, 3 * sizeof(std::uint32_t));

	std::span<std::uint32_t> indices = indexBuffer.cast();
	ASSERT_EQ(indices.size(), 3u);
	indices[0] = 5;
	indices[1] = 6;
	indices[2] = 7;

	const IndexBufferObject &readOnly = indexBuffer;
	std::span<const std::uint32_t> constIndices = readOnly.cast();
	ASSERT_EQ(constIndices.size(), 3u);
	EXPECT_EQ(constIndices[0], 5u);
	EXPECT_EQ(constIndices[2], 7u);
}

TEST(OpenGLIndexBufferObjectTest, CastHonorsConfiguredElementType)
{
	IndexBufferObject indexBuffer(BufferObject::Usage::StaticDraw, 4 * sizeof(std::uint16_t));
	indexBuffer.setElementType(GL_UNSIGNED_SHORT);

	std::span<std::uint16_t> indices = indexBuffer.cast<std::uint16_t>();
	ASSERT_EQ(indices.size(), 4u);

	EXPECT_THROW({ [[maybe_unused]] auto wide = indexBuffer.cast(); }, std::runtime_error);
}

