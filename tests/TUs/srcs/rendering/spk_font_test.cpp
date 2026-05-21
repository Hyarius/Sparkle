#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <fstream>

#include "opengl_wrapper_test_utils.hpp"
#include "rendering/spk_font.hpp"

namespace
{
	std::filesystem::path systemFontPath()
	{
		const std::filesystem::path arial = "C:/Windows/Fonts/arial.ttf";
		if (std::filesystem::exists(arial))
		{
			return arial;
		}

		const std::filesystem::path segoe = "C:/Windows/Fonts/segoeui.ttf";
		if (std::filesystem::exists(segoe))
		{
			return segoe;
		}

		return {};
	}

	std::vector<uint8_t> readBytes(const std::filesystem::path& p_path)
	{
		std::ifstream file(p_path, std::ios::binary);
		return std::vector<uint8_t>(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());
	}
}

TEST(FontSizeTest, DefaultSizeIsZero)
{
	spk::Font::Size size;

	EXPECT_EQ(size.glyph, 0u);
	EXPECT_EQ(size.outline, 0u);
}

TEST(FontSizeTest, SingleArgConstructorSetsGlyphAndZeroOutline)
{
	spk::Font::Size size(24);

	EXPECT_EQ(size.glyph, 24u);
	EXPECT_EQ(size.outline, 0u);
}

TEST(FontSizeTest, TwoArgConstructorSetsBothFields)
{
	spk::Font::Size size(24, 2);

	EXPECT_EQ(size.glyph, 24u);
	EXPECT_EQ(size.outline, 2u);
}

TEST(FontSizeTest, EqualityOperatorReturnsTrueForIdenticalSizes)
{
	spk::Font::Size a(16, 1);
	spk::Font::Size b(16, 1);

	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
}

TEST(FontSizeTest, InequalityOperatorReturnsTrueForDifferentSizes)
{
	spk::Font::Size a(16, 1);
	spk::Font::Size b(16, 2);

	EXPECT_TRUE(a != b);
	EXPECT_FALSE(a == b);
}

TEST(FontSizeTest, LessThanOperatorOrdersByGlyphThenOutline)
{
	spk::Font::Size smallSize(8, 0);
	spk::Font::Size mediumSameOutline(16, 0);
	spk::Font::Size mediumLargerOutline(16, 2);

	EXPECT_TRUE(smallSize < mediumSameOutline);
	EXPECT_FALSE(mediumSameOutline < smallSize);
	EXPECT_TRUE(mediumSameOutline < mediumLargerOutline);
	EXPECT_FALSE(mediumLargerOutline < mediumSameOutline);
}

TEST(FontSizeTest, SizeUsableAsMapKey)
{
	std::map<spk::Font::Size, int> sizeMap;

	sizeMap[spk::Font::Size(12, 0)] = 1;
	sizeMap[spk::Font::Size(16, 1)] = 2;
	sizeMap[spk::Font::Size(12, 0)] = 3;

	EXPECT_EQ(sizeMap.size(), 2u);
	EXPECT_EQ(sizeMap[spk::Font::Size(12, 0)], 3);
}

TEST(FontGlyphTest, RescaleUpdatesUVsProportionally)
{
	spk::Font::Glyph glyph;
	glyph.uvs[0] = {0.0f, 0.0f};
	glyph.uvs[1] = {1.0f, 0.0f};
	glyph.uvs[2] = {0.0f, 1.0f};
	glyph.uvs[3] = {1.0f, 1.0f};

	glyph.rescale({0.5f, 0.25f});

	EXPECT_NEAR(glyph.uvs[1].x, 0.5f, 1e-5f);
	EXPECT_NEAR(glyph.uvs[2].y, 0.25f, 1e-5f);
	EXPECT_NEAR(glyph.uvs[3].x, 0.5f, 1e-5f);
	EXPECT_NEAR(glyph.uvs[3].y, 0.25f, 1e-5f);
}

TEST(FontGlyphTest, IndexesOrderHasSixElements)
{
	EXPECT_EQ(spk::Font::Glyph::indexesOrder.size(), 6u);
}

TEST(FontTest, DefaultConstructionDoesNotThrow)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	EXPECT_NO_THROW(spk::Font font);
}

TEST(FontTest, LoadFromNonexistentFileThrows)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	EXPECT_THROW(spk::Font font("nonexistent_font.ttf"), std::runtime_error);
}

TEST(FontTest, SetPropertiesOnDefaultFontDoesNotThrow)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Font font;

	EXPECT_NO_THROW(font.setProperties(
		spk::Font::Filtering::Linear,
		spk::Font::Wrap::ClampToEdge,
		spk::Font::Mipmap::Enable));
}

TEST(FontTest, FromRawDataWithEmptyDataThrows)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	std::vector<uint8_t> emptyData;

	EXPECT_THROW(spk::Font::fromRawData(emptyData), std::runtime_error);
}

TEST(FontTest, LoadsSystemFontAndComputesGlyphMetrics)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	spk::Font font(fontPath);

	const spk::Vector2UInt charSize = font.computeCharSize(L'A', spk::Font::Size(24, 2));
	const spk::Vector2UInt stringSize = font.computeStringSize(L"Hello", spk::Font::Size(24, 2));
	const spk::Vector2Int baselineOffset = font.computeStringBaselineOffset(L"Hello", spk::Font::Size(24, 2));

	EXPECT_GT(charSize.x, 0u);
	EXPECT_GT(charSize.y, 0u);
	EXPECT_GT(stringSize.x, charSize.x);
	EXPECT_GT(stringSize.y, 0u);
	EXPECT_GE(baselineOffset.y, 0);
}

TEST(FontTest, FromRawDataLoadsGlyphsAndNotifiesAtlasSubscribers)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	spk::Font font = spk::Font::fromRawData(
		readBytes(fontPath),
		spk::Font::Filtering::Linear,
		spk::Font::Wrap::Repeat,
		spk::Font::Mipmap::Disable);
	spk::Font::Atlas& atlas = font.atlas(spk::Font::Size(18, 1));

	int notificationCount = 0;
	auto contract = atlas.subscribe([&]() { ++notificationCount; });

	atlas.loadGlyphs(L"AB");
	const spk::Font::Glyph& glyph = atlas[L'A'];
	atlas.synchronize();

	EXPECT_GE(notificationCount, 2);
	EXPECT_GT(glyph.size.x, 0u);
	EXPECT_NE(atlas.glId(), spk::OpenGL::Texture::InvalidGLId);
	EXPECT_FALSE(atlas.needsSynchronization());
}

TEST(FontTest, SetPropertiesPropagatesToExistingAtlas)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	spk::Font font(fontPath);
	spk::Font::Atlas& atlas = font.atlas(spk::Font::Size(16, 0));
	atlas.glyph(L'A');

	font.setProperties(
		spk::Font::Filtering::Linear,
		spk::Font::Wrap::ClampToBorder,
		spk::Font::Mipmap::Disable);

	EXPECT_EQ(atlas.filtering(), spk::Texture::Filtering::Linear);
	EXPECT_EQ(atlas.wrap(), spk::Texture::Wrap::ClampToBorder);
	EXPECT_EQ(atlas.mipmap(), spk::Texture::Mipmap::Disable);
}

TEST(FontTest, ComputeOptimalTextSizeHandlesEmptyAndNonEmptyStrings)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	spk::Font font(fontPath);

	const spk::Font::Size emptySize = font.computeOptimalTextSize(L"", 0.1f, {200, 50});
	const spk::Font::Size textSize = font.computeOptimalTextSize(L"Sparkle", 0.1f, {200, 50});

	EXPECT_EQ(emptySize.glyph, 40u);
	EXPECT_EQ(emptySize.outline, 5u);
	EXPECT_GT(textSize.glyph, 0u);
	EXPECT_GT(textSize.outline, 0u);
	EXPECT_LT(textSize.glyph + textSize.outline * 2, 200u);
}

TEST(FontTest, AtlasLoadsAllRenderableGlyphs)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	spk::Font font(fontPath);
	spk::Font::Atlas& atlas = font.atlas(spk::Font::Size(12, 0));

	EXPECT_NO_THROW(atlas.loadAllRenderableGlyphs());
	EXPECT_GT(atlas.computeCharSize(L'A').x, 0u);
}

#endif
