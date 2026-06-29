#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/spk_vertex_buffer_object.hpp"

namespace
{
	struct Vertex
	{
		float position[2] = {};
		float uv[2] = {};
	};

	static_assert(std::is_trivially_copyable_v<Vertex>);
	static_assert(sizeof(Vertex) == 16);
}

TEST(OpenGLVertexBufferObjectTest, CastSpansBufferAsVertexArray)
{
	spk::VertexBufferObject vertexBuffer(spk::BufferObject::Usage::DynamicDraw, 2 * sizeof(Vertex));

	std::span<Vertex> vertices = vertexBuffer.cast<Vertex>();
	ASSERT_EQ(vertices.size(), 2u);
	vertices[0].position[0] = 1.0f;
	vertices[1].uv[1] = 0.5f;

	const spk::VertexBufferObject &readOnly = vertexBuffer;
	std::span<const Vertex> constVertices = readOnly.cast<Vertex>();
	ASSERT_EQ(constVertices.size(), 2u);
	EXPECT_FLOAT_EQ(constVertices[0].position[0], 1.0f);
	EXPECT_FLOAT_EQ(constVertices[1].uv[1], 0.5f);
}

TEST(OpenGLVertexBufferObjectTest, CastThrowsWhenSizeIsNotAWholeMultiple)
{
	spk::VertexBufferObject vertexBuffer(spk::BufferObject::Usage::DynamicDraw, sizeof(Vertex) + 4);

	EXPECT_THROW({ [[maybe_unused]] auto vertices = vertexBuffer.cast<Vertex>(); }, std::runtime_error);
}

TEST(OpenGLVertexBufferObjectTest, DefaultsToArrayBufferTargetAndUploadsVertexData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::array<sparkle_test::TestVertex, 3> vertices =
		sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f});
	auto vertexBuffer = sparkle_test::makeTriangleVBO(vertices);

	EXPECT_EQ(vertexBuffer->target(), spk::BufferObject::Target::Array);
	vertexBuffer->activate(context.renderContext());

	std::array<sparkle_test::TestVertex, 3> gpuVertices = {};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpuVertices), gpuVertices.data());
	EXPECT_EQ(gpuVertices[0].position, vertices[0].position);
	EXPECT_EQ(gpuVertices[1].color, vertices[1].color);
}

