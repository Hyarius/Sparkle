#include <filesystem>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "utils/image_comparison_test_utils.hpp"
#include "utils/test_resource_path_utils.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path imageComparisonTempDirectory()
	{
		std::filesystem::path result = std::filesystem::temp_directory_path() / "sparkle_image_comparison_tests";
		std::filesystem::create_directories(result);
		return result;
	}

	[[nodiscard]] std::filesystem::path imageComparisonExpectedDirectory()
	{
		return imageComparisonTempDirectory();
	}

	[[nodiscard]] std::filesystem::path imageComparisonResultDirectory()
	{
		return imageComparisonTempDirectory();
	}

	void writePng(const std::filesystem::path& p_path, int p_width, int p_height, const std::vector<unsigned char>& p_pixels)
	{
		ASSERT_NE(stbi_write_png(p_path.string().c_str(), p_width, p_height, 4, p_pixels.data(), p_width * 4), 0);
	}
}

TEST(ImageComparisonTest, MatchingImagesLeaveNoArtifacts)
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
	EXPECT_FALSE(std::filesystem::exists(actualPath));
	EXPECT_FALSE(std::filesystem::exists(diffPath));
}

TEST(ImageComparisonTest, RgbDifferenceAtToleranceMatchesAndAboveToleranceDiffers)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "tolerance_boundary_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "tolerance_boundary_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "tolerance_boundary_diff.png";

	const std::vector<unsigned char> actualPixels = {
		104, 100, 100, 255,
		105, 100, 100, 255
	};
	const std::vector<unsigned char> expectedPixels = {
		100, 100, 100, 255,
		100, 100, 100, 255
	};

	writePng(actualPath, 2, 1, actualPixels);
	writePng(expectedPath, 2, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 1);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 1);

	EXPECT_EQ(diffPixels[0], 0);
	EXPECT_EQ(diffPixels[1], 0);
	EXPECT_EQ(diffPixels[2], 0);
	EXPECT_EQ(diffPixels[3], 255);

	const int redPixelOffset = 4;
	EXPECT_EQ(diffPixels[redPixelOffset + 0], 255);
	EXPECT_EQ(diffPixels[redPixelOffset + 1], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 2], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 3], 255);

	stbi_image_free(diffPixels);
}

TEST(ImageComparisonTest, TransparentPixelsIgnoreRgbDifferences)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "transparent_rgb_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "transparent_rgb_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "transparent_rgb_diff.png";

	const std::vector<unsigned char> actualPixels = {255, 0, 255, 0};
	const std::vector<unsigned char> expectedPixels = {0, 255, 0, 0};

	writePng(actualPath, 1, 1, actualPixels);
	writePng(expectedPath, 1, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0);
}

TEST(ImageComparisonTest, DimensionMismatchMarksOutOfOverlapPixels)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "dimension_mismatch_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "dimension_mismatch_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "dimension_mismatch_diff.png";

	const std::vector<unsigned char> actualPixels = {
		10, 20, 30, 255,
		40, 50, 60, 255,
		70, 80, 90, 255,
		100, 110, 120, 255
	};
	const std::vector<unsigned char> expectedPixels = {
		10, 20, 30, 255,
		40, 50, 60, 255
	};

	writePng(actualPath, 2, 2, actualPixels);
	writePng(expectedPath, 2, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.actualWidth, 2);
	EXPECT_EQ(result.actualHeight, 2);
	EXPECT_EQ(result.expectedWidth, 2);
	EXPECT_EQ(result.expectedHeight, 1);
	EXPECT_EQ(result.differentPixelCount, 2);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 2);

	for (int x = 0; x < 2; ++x)
	{
		const int matchingPixelOffset = x * 4;
		EXPECT_EQ(diffPixels[matchingPixelOffset + 0], 0);
		EXPECT_EQ(diffPixels[matchingPixelOffset + 1], 0);
		EXPECT_EQ(diffPixels[matchingPixelOffset + 2], 0);
		EXPECT_EQ(diffPixels[matchingPixelOffset + 3], 255);

		const int missingPixelOffset = (2 + x) * 4;
		EXPECT_EQ(diffPixels[missingPixelOffset + 0], 255);
		EXPECT_EQ(diffPixels[missingPixelOffset + 1], 0);
		EXPECT_EQ(diffPixels[missingPixelOffset + 2], 0);
		EXPECT_EQ(diffPixels[missingPixelOffset + 3], 255);
	}

	stbi_image_free(diffPixels);
}

TEST(ImageComparisonTest, AlphaDifferenceAboveToleranceDiffers)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "alpha_difference_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "alpha_difference_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "alpha_difference_diff.png";

	const std::vector<unsigned char> actualPixels = {10, 20, 30, 255};
	const std::vector<unsigned char> expectedPixels = {10, 20, 30, 246};

	writePng(actualPath, 1, 1, actualPixels);
	writePng(expectedPath, 1, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 1);
}

TEST(ImageComparisonTest, MissingFileThrows)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "missing_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "missing_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "missing_diff.png";

	std::filesystem::remove(actualPath);
	const std::vector<unsigned char> expectedPixels = {0, 0, 0, 255};
	writePng(expectedPath, 1, 1, expectedPixels);

	EXPECT_THROW(static_cast<void>(sparkle_test::compareImages(actualPath, expectedPath, diffPath)), std::runtime_error);
}

TEST(ImageComparisonTest, AllPixelsDifferentAreCounted)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "all_different_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "all_different_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "all_different_diff.png";

	const std::vector<unsigned char> actualPixels = {
		255, 0, 0, 255,
		0, 255, 0, 255,
		0, 0, 255, 255,
		255, 255, 255, 255
	};
	const std::vector<unsigned char> expectedPixels = {
		0, 0, 0, 255,
		0, 0, 0, 255,
		0, 0, 0, 255,
		0, 0, 0, 255
	};

	writePng(actualPath, 2, 2, actualPixels);
	writePng(expectedPath, 2, 2, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 4);
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
