#include <array>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace
{
	std::shared_ptr<spk::OpenGL::VertexArrayObject> makeQuadVAO()
	{
		const std::array<sparkle_test::TestVertex, 4> vertices = {
			sparkle_test::TestVertex{{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
			sparkle_test::TestVertex{{1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
			sparkle_test::TestVertex{{1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
			sparkle_test::TestVertex{{-1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}
		};

		auto vertexBuffer = std::make_shared<spk::OpenGL::VertexBufferObject>(
			spk::OpenGL::BufferObject::Usage::StaticDraw,
			sizeof(vertices));
		vertexBuffer->edit(vertices.data(), sizeof(vertices));

		auto vertexArray = std::make_shared<spk::OpenGL::VertexArrayObject>();
		vertexArray->addVertexBuffer(
			vertexBuffer,
			spk::OpenGL::VertexArrayObject::Attribute{
				.index = 0,
				.componentCount = 2,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(sparkle_test::TestVertex),
				.offset = offsetof(sparkle_test::TestVertex, position)
			});
		vertexArray->addVertexBuffer(
			vertexBuffer,
			spk::OpenGL::VertexArrayObject::Attribute{
				.index = 1,
				.componentCount = 3,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(sparkle_test::TestVertex),
				.offset = offsetof(sparkle_test::TestVertex, color)
			});

		return vertexArray;
	}

	std::shared_ptr<spk::OpenGL::IndexBufferObject> makeQuadIndexBuffer()
	{
		const std::array<std::uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};
		auto indexBuffer = std::make_shared<spk::OpenGL::IndexBufferObject>(
			spk::OpenGL::BufferObject::Usage::StaticDraw,
			sizeof(indices));
		indexBuffer->setElementType(GL_UNSIGNED_SHORT);
		indexBuffer->setCount(indices.size());
		indexBuffer->edit(indices.data(), sizeof(indices));
		return indexBuffer;
	}
}

TEST(OpenGLDrawElementsCommandTest, DrawsIndexedElementsFromIndexBuffer)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	const auto program = sparkle_test::makeColorProgram();
	const auto vertexArray = makeQuadVAO();
	const auto indexBuffer = makeQuadIndexBuffer();

	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ScissorCommand>(spk::Rect2D(0, 0, width, height), false);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::OpenGL::DrawElementsCommand>(spk::OpenGL::Primitive::Triangles, program, vertexArray, indexBuffer);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	glFinish();

	const std::filesystem::path actual = sparkle_test::resultImagePath("draw_elements_command_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("draw_elements_command_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("draw_elements_command_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {255, 0, 0, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches) << "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
	EXPECT_EQ(result.differentPixelCount, 0);
}

#endif
