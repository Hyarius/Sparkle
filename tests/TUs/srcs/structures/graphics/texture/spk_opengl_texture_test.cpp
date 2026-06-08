#include <gtest/gtest.h>


#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/texture/spk_texture.hpp"

using Texture = spk::Texture;

TEST(OpenGLTextureTest, DefaultConstructionProducesInvalidGLId)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture tex;

	EXPECT_EQ(tex.glId(), Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, HasApplicationTextureID)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture tex;

	EXPECT_NE(tex.id(), spk::Texture::InvalidID);
}

TEST(OpenGLTextureTest, SynchronizeAfterSetPixelsUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture tex;
	std::vector<uint8_t> pixels(4 * 4 * 4, 255);

	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA);
	ASSERT_TRUE(tex.needsSynchronization());

	tex.synchronize();

	EXPECT_FALSE(tex.needsSynchronization());
	EXPECT_NE(tex.glId(), Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, SynchronizeCalledTwiceDoesNotReuploadUnlessNewData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture tex;
	std::vector<uint8_t> pixels(2 * 2 * 3, 128);
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGB);
	tex.synchronize();

	GLuint firstId = tex.glId();

	tex.synchronize();

	EXPECT_EQ(tex.glId(), firstId);
}

TEST(OpenGLTextureTest, MoveConstructionPreservesGLIdAndInvalidatesSource)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture src;
	std::vector<uint8_t> pixels(2 * 2 * 4, 200);
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	src.synchronize();
	GLuint originalId = src.glId();
	ASSERT_NE(originalId, Texture::InvalidGLId);

	Texture dst(std::move(src));

	EXPECT_EQ(dst.glId(), originalId);
	EXPECT_EQ(src.glId(), Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, MoveAssignmentPreservesGLId)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture src;
	std::vector<uint8_t> pixels(2 * 2 * 4, 100);
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	src.synchronize();
	GLuint originalId = src.glId();

	Texture dst;
	dst = std::move(src);

	EXPECT_EQ(dst.glId(), originalId);
}

TEST(OpenGLTextureTest, ForceSynchronizationUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture tex;
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);

	tex.forceSynchronization();

	EXPECT_NE(tex.glId(), Texture::InvalidGLId);
	EXPECT_FALSE(tex.needsSynchronization());
}

TEST(OpenGLTextureTest, SynchronizeCoversSupportedPixelFormats)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	const struct FormatCase
	{
		spk::Texture::Format format;
		std::size_t bytesPerPixel;
	} cases[] = {
		{spk::Texture::Format::BGR, 3},
		{spk::Texture::Format::BGRA, 4},
		{spk::Texture::Format::GreyLevel, 1},
		{spk::Texture::Format::DualChannel, 2}
	};

	for (const FormatCase& formatCase : cases)
	{
		Texture tex;
		std::vector<uint8_t> pixels(formatCase.bytesPerPixel, 255);

		tex.setPixels(
			pixels,
			{1, 1},
			formatCase.format,
			spk::Texture::Filtering::Linear,
			spk::Texture::Wrap::ClampToBorder,
			spk::Texture::Mipmap::Disable);
		tex.synchronize();

		EXPECT_NE(tex.glId(), Texture::InvalidGLId);
		EXPECT_FALSE(tex.needsSynchronization());
	}
}

TEST(OpenGLTextureTest, SynchronizeReturnsWithoutUploadingInvalidTextureData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture empty;
	empty.synchronize();
	EXPECT_EQ(empty.glId(), Texture::InvalidGLId);

	Texture invalidFormat;
	invalidFormat.setPixels(std::vector<uint8_t>{255}, {1, 1}, spk::Texture::Format::Error);
	invalidFormat.synchronize();
	EXPECT_EQ(invalidFormat.glId(), Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, MoveAssignmentDeletesExistingTextureBeforeTakingSource)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	Texture src;
	src.setPixels(std::vector<uint8_t>(4, 80), {1, 1}, spk::Texture::Format::RGBA);
	src.synchronize();
	const GLuint srcId = src.glId();

	Texture dst;
	dst.setPixels(std::vector<uint8_t>(4, 160), {1, 1}, spk::Texture::Format::RGBA);
	dst.synchronize();
	ASSERT_NE(dst.glId(), Texture::InvalidGLId);

	dst = std::move(src);

	EXPECT_EQ(dst.glId(), srcId);
	EXPECT_EQ(src.glId(), Texture::InvalidGLId);
}

