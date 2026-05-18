#include <filesystem>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "image_comparison_test_utils.hpp"
#include "spk_widget_visual_test_helpers.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path imageComparisonExpectedDirectory()
	{
		std::filesystem::path result = spk::test::expectedDirectory() / "ImageComparison";
		std::filesystem::create_directories(result);
		return result;
	}

	[[nodiscard]] std::filesystem::path imageComparisonResultDirectory()
	{
		std::filesystem::path result = spk::test::resultDirectory() / "ImageComparison";
		std::filesystem::create_directories(result);
		return result;
	}
}

TEST(ImageComparisonTest, MatchingImagesProduceBlackDifferenceImage)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "matching_actual.png";
	const std::filesystem::path expectedPath = imageComparisonExpectedDirectory() / "matching_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "matching_diff.png";

	const std::vector<unsigned char> pixels = {
		10, 20, 30, 255,
		40, 50, 60, 255,
		70, 80, 90, 255,
		100, 110, 120, 255
	};

	ASSERT_NE(stbi_write_png(actualPath.string().c_str(), 2, 2, 4, pixels.data(), 2 * 4), 0);
	ASSERT_NE(stbi_write_png(expectedPath.string().c_str(), 2, 2, 4, pixels.data(), 2 * 4), 0);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 2);

	for (int index = 0; index < width * height * 4; index += 4)
	{
		EXPECT_EQ(diffPixels[index + 0], 0);
		EXPECT_EQ(diffPixels[index + 1], 0);
		EXPECT_EQ(diffPixels[index + 2], 0);
		EXPECT_EQ(diffPixels[index + 3], 255);
	}

	stbi_image_free(diffPixels);
}

TEST(ImageComparisonTest, DifferentPixelsAreMarkedInRed)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "different_actual.png";
	const std::filesystem::path expectedPath = imageComparisonExpectedDirectory() / "different_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "different_diff.png";

	const std::vector<unsigned char> actualPixels = {
		0, 0, 0, 255,
		255, 255, 255, 255,
		0, 0, 0, 255,
		0, 0, 0, 255
	};
	const std::vector<unsigned char> expectedPixels = {
		0, 0, 0, 255,
		0, 0, 0, 255,
		0, 0, 0, 255,
		0, 0, 0, 255
	};

	ASSERT_NE(stbi_write_png(actualPath.string().c_str(), 2, 2, 4, actualPixels.data(), 2 * 4), 0);
	ASSERT_NE(stbi_write_png(expectedPath.string().c_str(), 2, 2, 4, expectedPixels.data(), 2 * 4), 0);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 1);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 2);

	const int redPixelOffset = 4;
	EXPECT_EQ(diffPixels[redPixelOffset + 0], 255);
	EXPECT_EQ(diffPixels[redPixelOffset + 1], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 2], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 3], 255);

	stbi_image_free(diffPixels);
}
