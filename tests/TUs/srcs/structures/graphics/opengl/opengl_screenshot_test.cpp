#include <filesystem>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image_write.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "utils/test_resource_path_utils.hpp"

#include <GL/gl.h>

namespace
{
	[[nodiscard]] std::filesystem::path openglScreenshotExpectedDirectory()
	{
		std::filesystem::path result = spk::test::expectedDirectory() / "OpenGLScreenshot";
		std::filesystem::create_directories(result);
		return result;
	}

	[[nodiscard]] std::filesystem::path openglScreenshotResultDirectory()
	{
		std::filesystem::path result = spk::test::resultDirectory() / "OpenGLScreenshot";
		std::filesystem::create_directories(result);
		return result;
	}
}

TEST(OpenGLScreenshotTest, RuntimeSavesCurrentFramebufferToPng)
{
	constexpr int width = 16;
	constexpr int height = 16;

	const std::filesystem::path actualPath = openglScreenshotResultDirectory() / "green_actual.png";
	const std::filesystem::path expectedPath = openglScreenshotExpectedDirectory() / "green_expected.png";
	const std::filesystem::path diffPath = openglScreenshotResultDirectory() / "green_diff.png";

	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));

	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	context.gpuRuntime().waitUntilWorkDone();

	context.gpuRuntime().saveScreenshot(actualPath, spk::Rect2D(0, 0, width, height));

	std::vector<unsigned char> mutableExpectedPixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
	for (std::size_t index = 0; index < mutableExpectedPixels.size(); index += 4)
	{
		mutableExpectedPixels[index + 0] = 0;
		mutableExpectedPixels[index + 1] = 255;
		mutableExpectedPixels[index + 2] = 0;
		mutableExpectedPixels[index + 3] = 255;
	}
	ASSERT_NE(stbi_write_png(expectedPath.string().c_str(), width, height, 4, mutableExpectedPixels.data(), width * 4), 0);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_TRUE(result.matches) << "actual=[" << actualPath.string() << "] expected=[" << expectedPath.string() << "] diff=[" << diffPath.string()
								<< "]";
	EXPECT_EQ(result.differentPixelCount, 0);
}
