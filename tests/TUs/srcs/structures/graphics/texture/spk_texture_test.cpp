#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

#include "structures/graphics/spk_texture.hpp"

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

	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA, spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, spk::Texture::Mipmap::Enable);

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
	const auto srcId = src.id();

	spk::Texture dst(std::move(src));

	EXPECT_EQ(dst.id(), srcId);
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{3, 3}));
	EXPECT_EQ(src.id(), spk::Texture::InvalidID);
}

TEST(TextureTest, MovedFromTextureReportsDefaultReadableState)
{
	spk::Texture src;
	src.setPixels(makePixels(2, 2, 4), {2, 2}, spk::Texture::Format::RGBA);

	spk::Texture dst(std::move(src));
	(void)dst;

	EXPECT_EQ(src.version(), 0u);
	EXPECT_TRUE(src.pixels().empty());
	EXPECT_EQ(src.size(), (spk::Vector2UInt{0, 0}));
	EXPECT_EQ(src.format(), spk::Texture::Format::Error);
	EXPECT_EQ(src.clone().size(), (spk::Vector2UInt{0, 0}));
}

TEST(TextureTest, SetPropertiesOnMovedFromTextureRecreatesResource)
{
	spk::Texture src;
	spk::Texture dst(std::move(src));
	(void)dst;

	src.setProperties(
		spk::Texture::Filtering::Linear,
		spk::Texture::Wrap::Repeat,
		spk::Texture::Mipmap::Disable);

	EXPECT_NE(src.id(), spk::Texture::InvalidID);
	EXPECT_EQ(src.version(), 1u);
	EXPECT_EQ(src.filtering(), spk::Texture::Filtering::Linear);
	EXPECT_EQ(src.wrap(), spk::Texture::Wrap::Repeat);
	EXPECT_EQ(src.mipmap(), spk::Texture::Mipmap::Disable);
}

TEST(TextureTest, MoveAssignmentTransfersOwnership)
{
	spk::Texture src;
	auto pixels = makePixels(1, 1, 4);
	src.setPixels(pixels, {1, 1}, spk::Texture::Format::RGBA);
	const auto srcId = src.id();

	spk::Texture dst;
	dst = std::move(src);

	EXPECT_EQ(dst.id(), srcId);
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{1, 1}));
	EXPECT_EQ(src.id(), spk::Texture::InvalidID);
}

TEST(TextureTest, SelfAssignmentsLeaveTextureIntact)
{
	spk::Texture tex;
	const auto pixels = makePixels(1, 1, 4);
	tex.setPixels(pixels, {1, 1}, spk::Texture::Format::RGBA);
	const spk::Texture::ID originalId = tex.id();
	spk::Texture &sameTexture = tex;

	tex = sameTexture;

	EXPECT_EQ(tex.id(), originalId);
	EXPECT_EQ(tex.pixels(), pixels);

	tex = std::move(sameTexture);

	EXPECT_EQ(tex.id(), originalId);
	EXPECT_EQ(tex.pixels(), pixels);
}

TEST(TextureTest, CopyConstructionSharesResource)
{
	spk::Texture src;
	auto pixels = makePixels(2, 2, 4);
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA, spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, spk::Texture::Mipmap::Enable);

	spk::Texture dst(src);

	EXPECT_EQ(dst.id(), src.id());
	EXPECT_EQ(dst.pixels(), src.pixels());
	EXPECT_EQ(dst.size(), src.size());
	EXPECT_EQ(dst.format(), src.format());
	EXPECT_EQ(dst.filtering(), src.filtering());
	EXPECT_EQ(dst.wrap(), src.wrap());
	EXPECT_EQ(dst.mipmap(), src.mipmap());

	auto newPixels = makePixels(1, 1, 4);
	dst.setPixels(newPixels, {1, 1}, spk::Texture::Format::RGBA);

	EXPECT_EQ(src.size(), (spk::Vector2UInt{1, 1}));
	EXPECT_EQ(src.pixels(), newPixels);
}

TEST(TextureTest, CopyAssignmentSharesResource)
{
	spk::Texture src;
	auto pixels = makePixels(3, 3, 3);
	src.setPixels(pixels, {3, 3}, spk::Texture::Format::RGB);

	spk::Texture dst;
	dst = src;

	EXPECT_EQ(dst.id(), src.id());
	EXPECT_EQ(dst.pixels(), src.pixels());
	EXPECT_EQ(dst.size(), src.size());
	EXPECT_EQ(dst.format(), src.format());
}

TEST(TextureTest, CloneIsIndependentOfSource)
{
	spk::Texture src;
	auto pixels = makePixels(1, 1, 4);
	src.setPixels(pixels, {1, 1}, spk::Texture::Format::RGBA);

	spk::Texture dst = src.clone();

	auto newPixels = makePixels(4, 4, 4);
	src.setPixels(newPixels, {4, 4}, spk::Texture::Format::RGBA);

	EXPECT_NE(dst.id(), src.id());
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{1, 1}));
	EXPECT_EQ(dst.pixels(), pixels);
}

TEST(TextureTest, ClonePreservesMetadataWithNewIdentity)
{
	spk::Texture src;
	auto pixels = makePixels(2, 2, 4);
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA, spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, spk::Texture::Mipmap::Enable);

	spk::Texture dst = src.clone();

	EXPECT_NE(dst.id(), src.id());
	EXPECT_EQ(dst.pixels(), src.pixels());
	EXPECT_EQ(dst.size(), src.size());
	EXPECT_EQ(dst.format(), src.format());
	EXPECT_EQ(dst.filtering(), src.filtering());
	EXPECT_EQ(dst.wrap(), src.wrap());
	EXPECT_EQ(dst.mipmap(), src.mipmap());
}

TEST(TextureTest, CloneOfDefaultTextureKeepsEmptyResourceWithNewIdentity)
{
	spk::Texture src;

	spk::Texture dst = src.clone();

	EXPECT_NE(dst.id(), src.id());
	EXPECT_EQ(dst.version(), 0u);
	EXPECT_TRUE(dst.pixels().empty());
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{0, 0}));
}

TEST(TextureTest, CopyOfEmptyTextureSharesEmptyResource)
{
	spk::Texture src;

	spk::Texture dst(src);

	EXPECT_EQ(dst.id(), src.id());
	EXPECT_TRUE(dst.pixels().empty());
	EXPECT_EQ(dst.size(), (spk::Vector2UInt{0, 0}));
	EXPECT_FALSE(dst.needsSynchronization());
}

TEST(TextureTest, SetPixelsBumpsVersion)
{
	spk::Texture tex;
	EXPECT_EQ(tex.version(), 0u);

	auto pixels = makePixels(1, 1, 4);
	tex.setPixels(pixels, {1, 1}, spk::Texture::Format::RGBA);
	EXPECT_EQ(tex.version(), 1u);

	tex.setPixels(pixels, {1, 1}, spk::Texture::Format::RGBA);
	EXPECT_EQ(tex.version(), 2u);
}

TEST(TextureTest, SetPropertiesBumpsVersion)
{
	spk::Texture tex;
	tex.setProperties(spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, spk::Texture::Mipmap::Disable);
	EXPECT_EQ(tex.version(), 1u);
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

TEST(TextureTest, ReleasedTextureIdCanBeReused)
{
	spk::Texture::ID releasedId = spk::Texture::InvalidID;
	{
		spk::Texture tex;
		releasedId = tex.id();
	}

	spk::Texture next;

	EXPECT_EQ(next.id(), releasedId);
}

TEST(TextureTest, RawPointerOverloadsFillWithZeroesWhenDataIsNull)
{
	spk::Texture withProperties;
	withProperties.setPixels(
		nullptr,
		{2, 2},
		spk::Texture::Format::DualChannel,
		spk::Texture::Filtering::Linear,
		spk::Texture::Wrap::Repeat,
		spk::Texture::Mipmap::Disable);

	EXPECT_EQ(withProperties.pixels().size(), 8u);
	EXPECT_TRUE(std::ranges::all_of(withProperties.pixels(), [](uint8_t p_value) {
		return p_value == 0;
	}));
	EXPECT_EQ(withProperties.filtering(), spk::Texture::Filtering::Linear);
	EXPECT_EQ(withProperties.wrap(), spk::Texture::Wrap::Repeat);
	EXPECT_EQ(withProperties.mipmap(), spk::Texture::Mipmap::Disable);

	spk::Texture withoutProperties;
	withoutProperties.setPixels(nullptr, {1, 1}, spk::Texture::Format::BGRA);

	EXPECT_EQ(withoutProperties.pixels().size(), 4u);
	EXPECT_TRUE(std::ranges::all_of(withoutProperties.pixels(), [](uint8_t p_value) {
		return p_value == 0;
	}));
}

TEST(TextureTest, SectionInequalityDetectsDifferentAnchorAndSize)
{
	const spk::Texture::Section base{{0.0f, 0.0f}, {1.0f, 1.0f}};

	EXPECT_NE(base, (spk::Texture::Section{{0.25f, 0.0f}, {1.0f, 1.0f}}));
	EXPECT_NE(base, (spk::Texture::Section{{0.0f, 0.0f}, {0.5f, 1.0f}}));
}

TEST(TextureTest, SynchronizeClearsPendingFlagOnBaseTexture)
{
	spk::Texture tex;
	tex.setProperties(spk::Texture::Filtering::Linear, spk::Texture::Wrap::Repeat, spk::Texture::Mipmap::Disable);

	ASSERT_TRUE(tex.needsSynchronization());
	tex.synchronize();

	EXPECT_FALSE(tex.needsSynchronization());
}

TEST(TextureTest, SaveAsPngThrowsWithoutPixelData)
{
	const spk::Texture tex;

	EXPECT_THROW(tex.saveAsPng("empty-texture.png"), std::runtime_error);
}

TEST(TextureTest, SaveAsPngThrowsForUnsupportedFormat)
{
	spk::Texture tex;
	tex.setPixels(std::vector<uint8_t>{1, 2, 3, 4}, {1, 1}, spk::Texture::Format::Error);

	EXPECT_THROW(tex.saveAsPng("unsupported-texture.png"), std::runtime_error);
}

TEST(TextureTest, SaveAsPngThrowsWhenWidthOrHeightIsZero)
{
	spk::Texture zeroWidth;
	zeroWidth.setPixels(std::vector<uint8_t>{255, 0, 0, 255}, {0, 1}, spk::Texture::Format::RGBA);
	EXPECT_THROW(zeroWidth.saveAsPng("zero-width-texture.png"), std::runtime_error);

	spk::Texture zeroHeight;
	zeroHeight.setPixels(std::vector<uint8_t>{255, 0, 0, 255}, {1, 0}, spk::Texture::Format::RGBA);
	EXPECT_THROW(zeroHeight.saveAsPng("zero-height-texture.png"), std::runtime_error);
}

TEST(TextureTest, SaveAsPngWritesSupportedTexture)
{
	spk::Texture tex;
	tex.setPixels(std::vector<uint8_t>{255, 0, 0, 255}, {1, 1}, spk::Texture::Format::RGBA);

	const std::filesystem::path outputPath = std::filesystem::temp_directory_path() / "sparkle-texture-test.png";
	std::filesystem::remove(outputPath);

	ASSERT_NO_THROW(tex.saveAsPng(outputPath));
	EXPECT_TRUE(std::filesystem::exists(outputPath));

	std::filesystem::remove(outputPath);
}

TEST(TextureTest, SaveAsPngWritesBgrTextureWithoutParentPath)
{
	spk::Texture tex;
	tex.setPixels(std::vector<uint8_t>{255, 0, 0}, {1, 1}, spk::Texture::Format::BGR);

	const std::filesystem::path outputPath = "sparkle-texture-bgr-test.png";
	std::filesystem::remove(outputPath);

	ASSERT_NO_THROW(tex.saveAsPng(outputPath));
	EXPECT_TRUE(std::filesystem::exists(outputPath));

	std::filesystem::remove(outputPath);
}

TEST(TextureTest, SaveAsPngThrowsWhenTargetPathIsDirectory)
{
	spk::Texture tex;
	tex.setPixels(std::vector<uint8_t>{255, 0, 0, 255}, {1, 1}, spk::Texture::Format::RGBA);

	EXPECT_THROW(tex.saveAsPng(std::filesystem::temp_directory_path()), std::runtime_error);
}

TEST(TextureTest, FormatClassificationDoesNotInferRolesFromByteCounts)
{
	EXPECT_TRUE(spk::Texture::isColorFormat(spk::Texture::Format::RGBA));
	EXPECT_FALSE(spk::Texture::isDepthFormat(spk::Texture::Format::RGBA));
	EXPECT_TRUE(spk::Texture::isDepthFormat(spk::Texture::Format::Depth24));
	EXPECT_TRUE(spk::Texture::isDepthFormat(spk::Texture::Format::Depth32F));
	EXPECT_TRUE(spk::Texture::isDepthStencilFormat(spk::Texture::Format::Depth24Stencil8));
	EXPECT_FALSE(spk::Texture::isColorFormat(spk::Texture::Format::Depth24Stencil8));
}

TEST(TextureTest, CpuPixelUploadsRejectDepthRenderTargetFormats)
{
	spk::Texture texture;
	const std::vector<std::uint8_t> pixels(4, 0);

	EXPECT_THROW(
		texture.setPixels(pixels, {1, 1}, spk::Texture::Format::Depth24),
		std::invalid_argument);
	EXPECT_THROW(
		texture.setPixels(pixels, {1, 1}, spk::Texture::Format::Depth24Stencil8),
		std::invalid_argument);
}
