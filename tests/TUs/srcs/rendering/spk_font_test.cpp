#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl_wrapper_test_utils.hpp"
#include "rendering/spk_font.hpp"

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

#endif
