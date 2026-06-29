#include <gtest/gtest.h>


#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include <stb_image_write.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"

namespace
{
	std::vector<uint8_t> makePngBytes(int p_width, int p_height, int p_channels = 4, uint8_t p_fill = 200)
	{
		std::vector<uint8_t> pixels(p_width * p_height * p_channels, p_fill);
		std::vector<uint8_t> out;

		stbi_write_png_to_func(
			[](void* p_ctx, void* p_data, int p_size)
			{
				auto* buf = reinterpret_cast<std::vector<uint8_t>*>(p_ctx);
				const auto* bytes = reinterpret_cast<uint8_t*>(p_data);
				buf->insert(buf->end(), bytes, bytes + p_size);
			},
			&out, p_width, p_height, p_channels, pixels.data(), p_width * p_channels);

		return out;
	}

	spk::SpriteSheet makeSpriteSheet(const spk::Vector2UInt& p_spriteCount, int p_imgW = 64, int p_imgH = 64)
	{
		auto pngData = makePngBytes(p_imgW, p_imgH);
		spk::SpriteSheet sheet;
		sheet.loadFromData(pngData, p_spriteCount);
		return sheet;
	}
}

TEST(SpriteSheetTest, DefaultConstructionProducesEmptySheet)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::SpriteSheet sheet;

	EXPECT_EQ(sheet.size(), (spk::Vector2UInt{0, 0}));
	EXPECT_TRUE(sheet.sprites().empty());
}

TEST(SpriteSheetTest, LoadFromDataSetsCorrectSpriteCount)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({4, 4});

	EXPECT_EQ(sheet.nbSprite(), (spk::Vector2UInt{4, 4}));
	EXPECT_EQ(sheet.sprites().size(), 16u);
}

TEST(SpriteSheetTest, PathConstructorLoadsImageAndBuildsSprites)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::filesystem::path path =
		std::filesystem::temp_directory_path() /
		("sparkle_sprite_sheet_constructor_test_" + std::to_string(suffix) + ".png");
	const std::vector<uint8_t> pngData = makePngBytes(8, 8);

	{
		std::ofstream file(path, std::ios::binary);
		ASSERT_TRUE(file.is_open());
		file.write(reinterpret_cast<const char *>(pngData.data()), static_cast<std::streamsize>(pngData.size()));
	}

	spk::SpriteSheet sheet(path, {2, 2});
	std::filesystem::remove(path);

	EXPECT_EQ(sheet.nbSprite(), (spk::Vector2UInt{2, 2}));
	EXPECT_EQ(sheet.sprites().size(), 4u);
}

TEST(SpriteSheetTest, UnitIsReciprocaOfSpriteCount)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({4, 2});

	EXPECT_NEAR(sheet.unit().x, 0.25f, 1e-5f);
	EXPECT_NEAR(sheet.unit().y, 0.5f, 1e-5f);
}

TEST(SpriteSheetTest, SpriteIDFromCoordIsCorrect)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({4, 3});

	EXPECT_EQ(sheet.spriteID({0, 0}), 0u);
	EXPECT_EQ(sheet.spriteID({1, 0}), 1u);
	EXPECT_EQ(sheet.spriteID({0, 1}), 4u);
	EXPECT_EQ(sheet.spriteID({3, 2}), 11u);
}

TEST(SpriteSheetTest, SpriteByCoordMatchesSpriteByID)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({3, 3});

	EXPECT_EQ(sheet.sprite({1, 2}), sheet.sprite(sheet.spriteID({1, 2})));
}

TEST(SpriteSheetTest, SpritesAreEvenlySpaced)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({2, 2}, 64, 64);

	const auto& s00 = sheet.sprite({0, 0});
	const auto& s10 = sheet.sprite({1, 0});
	const auto& s01 = sheet.sprite({0, 1});

	// Each sprite is its cell (unit = 0.5) inset by half a texel on every
	// side, so the visible size is unit minus one full texel (1/64 here).
	EXPECT_NEAR(s00.size.x, 0.5f - 1.0f / 64.0f, 1e-4f);
	EXPECT_NEAR(s00.size.y, 0.5f - 1.0f / 64.0f, 1e-4f);
	EXPECT_GT(s10.anchor.x, s00.anchor.x);
	EXPECT_GT(s01.anchor.y, s00.anchor.y);
}

TEST(SpriteSheetTest, SpriteIDOutOfRangeThrows)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({2, 2});

	EXPECT_THROW(sheet.sprite(100u), std::out_of_range);
}

TEST(SpriteSheetTest, SpriteCoordOutOfRangeThrows)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({2, 2});

	EXPECT_THROW(sheet.sprite({5, 0}), std::out_of_range);
	EXPECT_THROW(sheet.sprite({0, 5}), std::out_of_range);
}

TEST(SpriteSheetTest, ZeroSpriteCountThrowsOnLoad)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(4, 4);

	spk::SpriteSheet sheet;
	EXPECT_THROW(sheet.loadFromData(pngData, {0, 1}), std::invalid_argument);
	EXPECT_THROW(sheet.loadFromData(pngData, {1, 0}), std::invalid_argument);
}

TEST(SpriteSheetTest, SingleSpriteCoversMostOfTexture)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto sheet = makeSpriteSheet({1, 1}, 64, 64);

	ASSERT_EQ(sheet.sprites().size(), 1u);
	const auto& sprite = sheet.sprites()[0];
	EXPECT_NEAR(sprite.size.x + sprite.anchor.x, 1.0f, 0.02f);
	EXPECT_NEAR(sprite.size.y + sprite.anchor.y, 1.0f, 0.02f);
}

TEST(SpriteSheetTest, FromRawDataFactoryCreatesValidSheet)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(8, 8);

	auto sheet = spk::SpriteSheet::fromRawData(pngData, {2, 2});

	EXPECT_EQ(sheet.nbSprite(), (spk::Vector2UInt{2, 2}));
	EXPECT_EQ(sheet.sprites().size(), 4u);
}

