#include <gtest/gtest.h>

#include "rendering/spk_texture.hpp"

namespace
{
	std::vector<uint8_t> makePixels(size_t p_width, size_t p_height, size_t p_channels)
	{
		std::vector<uint8_t> pixels(p_width * p_height * p_channels, 128);
		return pixels;
	}
}

TEST(TextureTest, DefaultConstructionProducesEmptyTexture)
{
	spk::Texture tex;

	EXPECT_EQ(tex.size(), (spk::Vector2UInt{0, 0}));
	EXPECT_TRUE(tex.pixels().empty());
}

TEST(TextureTest, UniqueIDsAssignedOnConstruction)
{
	spk::Texture a;
	spk::Texture b;

	EXPECT_NE(a.id(), spk::Texture::InvalidID);
	EXPECT_NE(b.id(), spk::Texture::InvalidID);
	EXPECT_NE(a.id(), b.id());
}

TEST(TextureTest, SetPixelsStoresDataAndMetadata)
{
	spk::Texture tex;
	auto pixels = makePixels(4, 4, 4);

	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA,
		spk::Texture::Filtering::Linear,
		spk::Texture::Wrap::Repeat,
		spk::Texture::Mipmap::Enable);

	EXPECT_EQ(tex.size(), (spk::Vector2UInt{4, 4}));
	EXPECT_EQ(tex.pixels(), pixels);
	EXPECT_EQ(tex.format(), spk::Texture::Format::RGBA);
	EXPECT_EQ(tex.filtering(), spk::Texture::Filtering::Linear);
	EXPECT_EQ(tex.wrap(), spk::Texture::Wrap::Repeat);
	EXPECT_EQ(tex.mipmap(), spk::Texture::Mipmap::Enable);
}

TEST(TextureTest, SetPixelsShortOverloadPreservesExistingProperties)
{
	spk::Texture tex;
	auto pixels = makePixels(2, 2, 3);

	tex.setProperties(spk::Texture::Filtering::Nearest, spk::Texture::Wrap::ClampToEdge, spk::Texture::Mipmap::Disable);
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGB);

	EXPECT_EQ(tex.filtering(), spk::Texture::Filtering::Nearest);
	EXPECT_EQ(tex.wrap(), spk::Texture::Wrap::ClampToEdge);
	EXPECT_EQ(tex.mipmap(), spk::Texture::Mipmap::Disable);
	EXPECT_EQ(tex.format(), spk::Texture::Format::RGB);
}

TEST(TextureTest, SetPixelsRawPointerOverloadStoresData)
{
	spk::Texture tex;
	auto pixels = makePixels(2, 2, 1);

	tex.setPixels(pixels.data(), {2, 2}, spk::Texture::Format::GreyLevel);

	EXPECT_EQ(tex.size(), (spk::Vector2UInt{2, 2}));
	EXPECT_EQ(tex.pixels().size(), 4u);
}

TEST(TextureTest, SetPropertiesUpdatesFilteringWrapAndMipmap)
{
	spk::Texture tex;

	tex.setProperties(spk::Texture::Filtering::Linear, spk::Texture::Wrap::ClampToBorder, spk::Texture::Mipmap::Enable);

	EXPECT_EQ(tex.filtering(), spk::Texture::Filtering::Linear);
	EXPECT_EQ(tex.wrap(), spk::Texture::Wrap::ClampToBorder);
	EXPECT_EQ(tex.mipmap(), spk::Texture::Mipmap::Enable);
}

TEST(TextureTest, MoveConstructionTransfersOwnership)
{
	spk::Texture src;
	auto pixels = makePixels(3, 3, 4);
	src.setPixels(pixels, {3, 3}, spk::Texture::Format::RGBA);
	auto srcId = src.id();

	spk::Texture dst(std::move(src));

	EXPECT_EQ(dst.id(), srcId);
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{3, 3}));
}

TEST(TextureTest, MoveAssignmentTransfersOwnership)
{
	spk::Texture src;
	auto pixels = makePixels(1, 1, 4);
	src.setPixels(pixels, {1, 1}, spk::Texture::Format::RGBA);
	auto srcId = src.id();

	spk::Texture dst;
	dst = std::move(src);

	EXPECT_EQ(dst.id(), srcId);
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{1, 1}));
}

TEST(TextureTest, SectionDefaultConstructionProducesZeroAnchorAndSize)
{
	spk::Texture::Section section;

	EXPECT_EQ(section.anchor, (spk::Vector2{0.0f, 0.0f}));
	EXPECT_EQ(section.size, (spk::Vector2{0.0f, 0.0f}));
}

TEST(TextureTest, SectionEqualityOperatorsWork)
{
	spk::Texture::Section a{{0.0f, 0.0f}, {1.0f, 1.0f}};
	spk::Texture::Section b{{0.0f, 0.0f}, {1.0f, 1.0f}};
	spk::Texture::Section c{{0.5f, 0.5f}, {0.5f, 0.5f}};

	EXPECT_EQ(a, b);
	EXPECT_NE(a, c);
}

TEST(TextureTest, SectionWholeCoversFullTexture)
{
	EXPECT_EQ(spk::Texture::Section::whole.anchor, (spk::Vector2{0.0f, 0.0f}));
	EXPECT_EQ(spk::Texture::Section::whole.size, (spk::Vector2{1.0f, 1.0f}));
}

TEST(TextureTest, SetPixelsMarksSynchronizationAsPending)
{
	spk::Texture tex;
	auto pixels = makePixels(2, 2, 4);

	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);

	EXPECT_TRUE(tex.needsSynchronization());
}
