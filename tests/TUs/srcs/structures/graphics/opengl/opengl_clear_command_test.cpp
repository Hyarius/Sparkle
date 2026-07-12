#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "utils/image_comparison_test_utils.hpp"

TEST(OpenGLClearCommandTest, ClearsFramebufferToRequestedColor)
{
	constexpr int width = 8;
	constexpr int height = 8;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext &renderContext = context.renderContext();

	spk::ClearCommand clearCommand(std::array<float, 4>{0.0f, 1.0f, 0.0f, 1.0f});
	clearCommand.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::resultImagePath("clear_command_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("clear_command_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("clear_command_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {0, 255, 0, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches);
}

TEST(OpenGLClearCommandTest, RestoresClearColorAndDepthAndStencilWriteMasks)
{
	sparkle_test::OpenGLTestContext context;
	spk::RenderContext &renderContext = context.renderContext();
	glClearColor(0.2f, 0.3f, 0.4f, 0.5f);
	glDepthMask(GL_FALSE);
	glStencilMaskSeparate(GL_FRONT, 0x12u);
	glStencilMaskSeparate(GL_BACK, 0x34u);

	spk::ClearCommand clear(
		{0.8f, 0.7f, 0.6f, 0.5f},
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	clear.execute(renderContext);

	GLfloat clearColor[4]{};
	GLboolean depthMask = GL_TRUE;
	GLint frontStencilMask = 0;
	GLint backStencilMask = 0;
	glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
	glGetIntegerv(GL_STENCIL_WRITEMASK, &frontStencilMask);
	glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &backStencilMask);
	EXPECT_FLOAT_EQ(clearColor[0], 0.2f);
	EXPECT_FLOAT_EQ(clearColor[1], 0.3f);
	EXPECT_FLOAT_EQ(clearColor[2], 0.4f);
	EXPECT_FLOAT_EQ(clearColor[3], 0.5f);
	EXPECT_EQ(depthMask, GL_FALSE);
	EXPECT_EQ(frontStencilMask, 0x12);
	EXPECT_EQ(backStencilMask, 0x34);

	glDepthMask(GL_TRUE);
	glStencilMask(0xFFu);
}
