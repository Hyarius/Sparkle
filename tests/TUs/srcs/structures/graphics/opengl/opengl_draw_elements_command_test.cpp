#include <array>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"


#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"

namespace
{
	std::shared_ptr<spk::VertexArrayObject> makeQuadVAO()
	{
		const std::array<sparkle_test::TestVertex, 4> vertices = {
			sparkle_test::TestVertex{{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
			sparkle_test::TestVertex{{1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
			sparkle_test::TestVertex{{1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
			sparkle_test::TestVertex{{-1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}
		};

		auto vertexBuffer = std::make_shared<spk::VertexBufferObject>(
			spk::BufferObject::Usage::StaticDraw,
			sizeof(vertices));
		vertexBuffer->edit(vertices.data(), sizeof(vertices));

		auto vertexArray = std::make_shared<spk::VertexArrayObject>();
		vertexArray->addVertexBuffer(
			vertexBuffer,
			spk::VertexArrayObject::Attribute{
				.index = 0,
				.componentCount = 2,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(sparkle_test::TestVertex),
				.offset = offsetof(sparkle_test::TestVertex, position)
			});
		vertexArray->addVertexBuffer(
			vertexBuffer,
			spk::VertexArrayObject::Attribute{
				.index = 1,
				.componentCount = 3,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(sparkle_test::TestVertex),
				.offset = offsetof(sparkle_test::TestVertex, color)
			});

		return vertexArray;
	}

	std::shared_ptr<spk::IndexBufferObject> makeQuadIndexBuffer()
	{
		const std::array<std::uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};
		auto indexBuffer = std::make_shared<spk::IndexBufferObject>(
			spk::BufferObject::Usage::StaticDraw,
			sizeof(indices));
		indexBuffer->setElementType(GL_UNSIGNED_SHORT);
		indexBuffer->edit(indices.data(), sizeof(indices));
		return indexBuffer;
	}
}

TEST(OpenGLDrawElementsCommandTest, DrawsIndexedElementsFromIndexBuffer)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	const auto program = sparkle_test::makeColorProgram();
	const auto vertexArray = makeQuadVAO();
	const auto indexBuffer = makeQuadIndexBuffer();

	spk::RenderUnitBuilder builder;
	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawElementsCommand>(spk::Primitive::Triangles, program, vertexArray, indexBuffer);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::resultImagePath("draw_elements_command_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("draw_elements_command_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("draw_elements_command_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {255, 0, 0, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches) << "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
	EXPECT_EQ(result.differentPixelCount, 0);
}

TEST(OpenGLDrawElementsCommandTest, ThrowsWhenIndexBufferIsMissing)
{
	sparkle_test::OpenGLTestContext context;
	spk::DrawElementsCommand command(spk::Primitive::Triangles, nullptr);

	EXPECT_THROW(command.execute(context.renderContext()), std::runtime_error);
}

