#include <gtest/gtest.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"


#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"

TEST(OpenGLVertexArrayObjectTest, ActivatesConfiguredVertexAttributes)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto vertexArray = sparkle_test::makeTriangleVAO(
		sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f}));
	vertexArray->activate();

	GLint currentVertexArray = 0;
	GLint positionEnabled = 0;
	GLint colorEnabled = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVertexArray);
	glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &positionEnabled);
	glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &colorEnabled);

	EXPECT_EQ(static_cast<GLuint>(currentVertexArray), vertexArray->id());
	EXPECT_EQ(positionEnabled, GL_TRUE);
	EXPECT_EQ(colorEnabled, GL_TRUE);
}

TEST(OpenGLVertexArrayObjectTest, RendersTriangleWithConfiguredVBO)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	const auto program = sparkle_test::makeColorProgram();
	const auto vertexArray = sparkle_test::makeTriangleVAO(
		sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f}));

	spk::RenderUnitBuilder builder;
	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawArraysCommand>(spk::Primitive::Triangles, program, vertexArray, 0, 3);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::resultImagePath("vao_triangle_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("vao_triangle_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("vao_triangle_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {0, 255, 0, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches) << "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
}

TEST(OpenGLVertexArrayObjectTest, MutationHelpersRequestSynchronization)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto vertexArray = sparkle_test::makeTriangleVAO(
		sparkle_test::fullScreenTriangle({1.0f, 0.0f, 0.0f}));
	vertexArray->activate();
	ASSERT_TRUE(vertexArray->isAllocated());
	ASSERT_FALSE(vertexArray->needsSynchronization());

	vertexArray->clearVertexBuffers();
	EXPECT_TRUE(vertexArray->needsSynchronization());
	vertexArray->activate();
	EXPECT_FALSE(vertexArray->needsSynchronization());

	auto indexBuffer = std::make_shared<spk::IndexBufferObject>(
		spk::BufferObject::Usage::StaticDraw,
		0);
	vertexArray->setIndexBuffer(indexBuffer);
	EXPECT_EQ(vertexArray->indexBuffer(), indexBuffer);
	EXPECT_TRUE(vertexArray->needsSynchronization());

	vertexArray->clearIndexBuffer();
	EXPECT_EQ(vertexArray->indexBuffer(), nullptr);
	EXPECT_TRUE(vertexArray->needsSynchronization());

	vertexArray->deactivate();
	GLint currentVertexArray = 1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVertexArray);
	EXPECT_EQ(currentVertexArray, 0);
}

TEST(OpenGLVertexArrayObjectTest, RejectsNullVertexBuffer)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::VertexArrayObject vertexArray;

	EXPECT_THROW(
		vertexArray.addVertexBuffer(
			nullptr,
			spk::VertexArrayObject::Attribute{
				.index = 0,
				.componentCount = 2,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(float) * 2,
				.offset = 0
			}),
		std::runtime_error);
}

