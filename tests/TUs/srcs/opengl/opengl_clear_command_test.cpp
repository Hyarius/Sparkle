#include <gtest/gtest.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"


TEST(OpenGLClearCommandTest, ClearsFramebufferToRequestedColor)
{
	constexpr int width = 8;
	constexpr int height = 8;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

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

