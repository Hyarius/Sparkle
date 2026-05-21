#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stb_image_write.h>

#include "opengl_wrapper_test_utils.hpp"
#include "rendering/spk_image.hpp"

namespace
{
	std::vector<uint8_t> makePngBytes(int p_width, int p_height, int p_channels, uint8_t p_fillValue = 200)
	{
		std::vector<uint8_t> pixels(p_width * p_height * p_channels, p_fillValue);
		std::vector<uint8_t> pngData;

		stbi_write_png_to_func(
			[](void* p_ctx, void* p_data, int p_size)
			{
				auto* buf = reinterpret_cast<std::vector<uint8_t>*>(p_ctx);
				const auto* bytes = reinterpret_cast<uint8_t*>(p_data);
				buf->insert(buf->end(), bytes, bytes + p_size);
			},
			&pngData,
			p_width,
			p_height,
			p_channels,
			pixels.data(),
			p_width * p_channels);

		return pngData;
	}
}

TEST(ImageTest, DefaultConstructionProducesEmptyImage)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Image img;

	EXPECT_EQ(img.size(), (spk::Vector2UInt{0, 0}));
	EXPECT_TRUE(img.pixels().empty());
	EXPECT_EQ(img.glId(), spk::OpenGL::Texture::InvalidGLId);
}

TEST(ImageTest, LoadFromDataRGBASetsCorrectSizeAndFormat)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(4, 4, 4);

	spk::Image img;
	EXPECT_NO_THROW(img.loadFromData(pngData));

	EXPECT_EQ(img.size(), (spk::Vector2UInt{4, 4}));
	EXPECT_EQ(img.format(), spk::Texture::Format::RGBA);
	EXPECT_FALSE(img.pixels().empty());
}

TEST(ImageTest, LoadFromDataRGBSetsCorrectFormat)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(2, 2, 3);

	spk::Image img;
	img.loadFromData(pngData);

	EXPECT_EQ(img.format(), spk::Texture::Format::RGB);
	EXPECT_EQ(img.size(), (spk::Vector2UInt{2, 2}));
}

TEST(ImageTest, LoadFromDataGreyLevelSetsCorrectFormat)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(1, 1, 1);

	spk::Image img;
	img.loadFromData(pngData);

	EXPECT_EQ(img.format(), spk::Texture::Format::GreyLevel);
}

TEST(ImageTest, LoadFromDataMarksSynchronizationAsPending)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(2, 2, 4);

	spk::Image img;
	img.loadFromData(pngData);

	EXPECT_TRUE(img.needsSynchronization());
}

TEST(ImageTest, SynchronizeAfterLoadUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto pngData = makePngBytes(2, 2, 4);

	spk::Image img;
	img.loadFromData(pngData);
	img.synchronize();

	EXPECT_NE(img.glId(), spk::OpenGL::Texture::InvalidGLId);
	EXPECT_FALSE(img.needsSynchronization());
}

TEST(ImageTest, LoadFromDataInvalidDataThrows)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const std::vector<uint8_t> badData = {0x00, 0x01, 0x02, 0x03};

	spk::Image img;
	EXPECT_THROW(img.loadFromData(badData), std::runtime_error);
}

TEST(ImageTest, ConstructorWithPathThrowsForMissingFile)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	EXPECT_THROW(spk::Image img("nonexistent_file_xyz.png"), std::runtime_error);
}

TEST(ImageTest, InheritsUniqueTextureID)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Image a;
	spk::Image b;

	EXPECT_NE(a.id(), spk::Texture::InvalidID);
	EXPECT_NE(b.id(), spk::Texture::InvalidID);
	EXPECT_NE(a.id(), b.id());
}

#endif
