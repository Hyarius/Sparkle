#include <array>

#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLVertexBufferObjectTest, DefaultsToArrayBufferTargetAndUploadsVertexData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::array<sparkle_test::TestVertex, 3> vertices =
		sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f});
	auto vertexBuffer = sparkle_test::makeTriangleVBO(vertices);

	EXPECT_EQ(vertexBuffer->target(), spk::OpenGL::BufferObject::Target::Array);
	vertexBuffer->activate();

	std::array<sparkle_test::TestVertex, 3> gpuVertices = {};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(gpuVertices), gpuVertices.data());
	EXPECT_EQ(gpuVertices[0].position, vertices[0].position);
	EXPECT_EQ(gpuVertices[1].color, vertices[1].color);
}

#endif
