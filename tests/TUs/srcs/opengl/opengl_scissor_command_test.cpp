#include <gtest/gtest.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLScissorCommandTest, AppliesRectAndCanDisableScissorTest)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	spk::OpenGL::ScissorCommand command(spk::Rect2D(3, 4, 5, 6));
	command.execute(renderContext);

	GLint scissor[4] = {};
	glGetIntegerv(GL_SCISSOR_BOX, scissor);
	EXPECT_EQ(glIsEnabled(GL_SCISSOR_TEST), GL_TRUE);
	EXPECT_EQ(scissor[0], 3);
	EXPECT_EQ(scissor[1], 4);
	EXPECT_EQ(scissor[2], 5);
	EXPECT_EQ(scissor[3], 6);

	spk::OpenGL::ScissorCommand disableCommand(spk::Rect2D(), false);
	disableCommand.execute(renderContext);
	EXPECT_EQ(glIsEnabled(GL_SCISSOR_TEST), GL_FALSE);
}

TEST(OpenGLScissorCommandTest, ClipsClearCommandToRequestedRectangle)
{
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ScissorCommand>(spk::Rect2D(0, 0, width, height), false);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 1.0f, 1.0f});
	builder.emplace<spk::OpenGL::ScissorCommand>(spk::Rect2D(4, 4, 8, 8));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{1.0f, 0.0f, 0.0f, 1.0f});

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	glFinish();

	const std::filesystem::path actual = sparkle_test::resultImagePath("scissor_command_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("scissor_command_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("scissor_command_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeScissorExpectedImage(expected, width, height);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches);
}

#endif
