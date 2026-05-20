#include <gtest/gtest.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLVertexArrayObjectTest, BindsConfiguredVertexAttributes)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto vertexArray = sparkle_test::makeTriangleVAO(
		sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f}));
	vertexArray->bind();

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
	spk::IRenderContext& renderContext = context.renderContext();

	const auto program = sparkle_test::makeColorProgram();
	const auto vertexArray = sparkle_test::makeTriangleVAO(
		sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f}));

	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ScissorCommand>(spk::Rect2D(0, 0, width, height), false);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::OpenGL::UseProgramRenderCommand>(program);
	builder.emplace<spk::OpenGL::BindVertexArrayCommand>(vertexArray);
	builder.emplace<spk::OpenGL::DrawArraysCommand>(spk::OpenGL::Primitive::Triangles, 0, 3);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	glFinish();

	const std::filesystem::path actual = sparkle_test::resultImagePath("vao_triangle_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("vao_triangle_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("vao_triangle_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {0, 255, 0, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches) << "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
}

#endif
