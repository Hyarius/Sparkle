#include <gtest/gtest.h>


#include <filesystem>
#include <vector>

#include "image_comparison_test_utils.hpp"
#include "rendering/spk_font.hpp"
#include "spk_generated_resources.hpp"
#include "test_resource_path_utils.hpp"

namespace
{
	[[nodiscard]] spk::Font::Text glyphSet()
	{
		spk::Font::Text result;
		for (spk::Font::Codepoint glyph = U'A'; glyph <= U'G'; ++glyph)
		{
			result.push_back(glyph);
		}
		return result;
	}

	[[nodiscard]] spk::Font loadPlaygroundFont()
	{
		const auto& bytes = SPARKLE_GET_RESOURCE("resources/fonts/arial.ttf");
		return spk::Font::fromRawData(std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
	}

	[[nodiscard]] std::filesystem::path fontTextureExpectedPath(const std::string& p_name)
	{
		return spk::test::expectedImagePath("FontTexture", p_name);
	}

	[[nodiscard]] std::filesystem::path fontTextureResultPath(const std::string& p_name)
	{
		return spk::test::resultImagePath("FontTexture", p_name);
	}
}

TEST(FontTextureTest, AtlasMatchesExpectedImage)
{
	spk::Font font = loadPlaygroundFont();
	spk::Font::Atlas& atlas = font.atlas(spk::Font::Size(32, 5));
	atlas.loadGlyphs(glyphSet());

	const std::filesystem::path actualPath = fontTextureResultPath("font_atlas_actual");
	const std::filesystem::path expectedPath = fontTextureExpectedPath("font_atlas_expected");
	const std::filesystem::path diffPath = fontTextureResultPath("font_atlas_diff");

	ASSERT_NO_THROW(atlas.saveAsPng(actualPath));
	ASSERT_GT(atlas.size().x, 0u);
	ASSERT_GT(atlas.size().y, 0u);
	ASSERT_FALSE(atlas.pixels().empty());

	if (std::filesystem::exists(expectedPath) == false)
	{
		ADD_FAILURE() << "Missing expected font atlas image at " << expectedPath.string()
					  << ". Saved actual output to " << actualPath.string()
					  << "; copy it to the expected path to establish the baseline.";
		return;
	}

	const sparkle_test::ImageComparisonResult result =
		sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_TRUE(result.matches)
		<< "actual=[" << actualPath.string() << "] expected=[" << expectedPath.string()
		<< "] diff=[" << diffPath.string() << "] differentPixels=" << result.differentPixelCount
		<< " actualSize=" << result.actualWidth << "x" << result.actualHeight
		<< " expectedSize=" << result.expectedWidth << "x" << result.expectedHeight;
}

