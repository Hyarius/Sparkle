#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/texture/spk_texture.hpp"

TEST(OpenGLTextureTest, DefaultConstructionProducesNoGPUUpload)
{
	sparkle_test::OpenGLTestContext context;

	spk::Texture tex;

	EXPECT_FALSE(context.renderContext().hasCompiledTexture(tex));
}

TEST(OpenGLTextureTest, HasApplicationTextureID)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Texture tex;

	EXPECT_NE(tex.id(), spk::Texture::InvalidID);
}

TEST(OpenGLTextureTest, CompiledTextureAfterSetPixelsHasValidGLId)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(4 * 4 * 4, 255);

	spk::Texture tex;
	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA);

	spk::OpenGL::Texture& glTex = context.renderContext().compiledTexture(tex);

	EXPECT_NE(glTex.id(), 0u);
	EXPECT_TRUE(context.renderContext().hasCompiledTexture(tex));
}

TEST(OpenGLTextureTest, SynchronizeWithContextUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(4 * 4 * 4, 255);

	spk::Texture tex;
	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA);
	ASSERT_TRUE(tex.needsSynchronization());

	tex.synchronize();

	EXPECT_FALSE(tex.needsSynchronization());
	EXPECT_TRUE(context.renderContext().hasCompiledTexture(tex));
	EXPECT_NE(context.renderContext().compiledTexture(tex).id(), 0u);
}

TEST(OpenGLTextureTest, CompiledTextureSameVersionReturnsCachedEntry)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 3, 128);

	spk::Texture tex;
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGB);

	const GLuint firstId = context.renderContext().compiledTexture(tex).id();
	const GLuint secondId = context.renderContext().compiledTexture(tex).id();

	EXPECT_EQ(firstId, secondId);
}

TEST(OpenGLTextureTest, SetPixelsBumpsVersionAndInvalidatesCache)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 200);

	spk::Texture tex;
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const GLuint firstId = context.renderContext().compiledTexture(tex).id();

	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const GLuint secondId = context.renderContext().compiledTexture(tex).id();

	EXPECT_NE(firstId, secondId);
}

TEST(OpenGLTextureTest, CopyProducesIndependentGPUTexture)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 100);

	spk::Texture src;
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const GLuint srcId = context.renderContext().compiledTexture(src).id();

	spk::Texture dst(src);
	const GLuint dstId = context.renderContext().compiledTexture(dst).id();

	EXPECT_NE(dst.id(), src.id());
	EXPECT_NE(dst.key(), src.key());
	EXPECT_NE(srcId, dstId);
	EXPECT_NE(dstId, 0u);
}

TEST(OpenGLTextureTest, MoveTransfersCacheKeyToDestination)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 200);

	spk::Texture src;
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const std::uint64_t originalKey = src.key();
	const GLuint originalId = context.renderContext().compiledTexture(src).id();
	ASSERT_NE(originalId, 0u);

	spk::Texture dst(std::move(src));

	EXPECT_EQ(dst.key(), originalKey);
	EXPECT_EQ(src.key(), 0u);
	EXPECT_EQ(context.renderContext().compiledTexture(dst).id(), originalId);
}

TEST(OpenGLTextureTest, ForceSynchronizationUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);

	spk::Texture tex;
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);

	tex.forceSynchronization();

	EXPECT_FALSE(tex.needsSynchronization());
	EXPECT_NE(context.renderContext().compiledTexture(tex).id(), 0u);
}

TEST(OpenGLTextureTest, CompiledTextureCoversAllSupportedPixelFormats)
{
	sparkle_test::OpenGLTestContext context;

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

	for (const FormatCase& fc : cases)
	{
		spk::Texture tex;
		std::vector<uint8_t> pixels(fc.bytesPerPixel, 255);
		tex.setPixels(
			pixels,
			{1, 1},
			fc.format,
			spk::Texture::Filtering::Linear,
			spk::Texture::Wrap::ClampToBorder,
			spk::Texture::Mipmap::Disable);

		EXPECT_NE(context.renderContext().compiledTexture(tex).id(), 0u);
	}
}

TEST(OpenGLTextureTest, EmptyTextureProducesGLIdZero)
{
	sparkle_test::OpenGLTestContext context;

	spk::Texture empty;
	EXPECT_EQ(context.renderContext().compiledTexture(empty).id(), 0u);

	spk::Texture invalidFormat;
	invalidFormat.setPixels(std::vector<uint8_t>{255}, {1, 1}, spk::Texture::Format::Error);
	EXPECT_EQ(context.renderContext().compiledTexture(invalidFormat).id(), 0u);
}

TEST(OpenGLTextureTest, MoveAssignmentTransfersCacheKey)
{
	sparkle_test::OpenGLTestContext context;

	spk::Texture src;
	src.setPixels(std::vector<uint8_t>(4, 80), {1, 1}, spk::Texture::Format::RGBA);
	const std::uint64_t srcKey = src.key();
	const GLuint srcGlId = context.renderContext().compiledTexture(src).id();

	spk::Texture dst;
	dst.setPixels(std::vector<uint8_t>(4, 160), {1, 1}, spk::Texture::Format::RGBA);
	(void)context.renderContext().compiledTexture(dst);

	dst = std::move(src);

	EXPECT_EQ(dst.key(), srcKey);
	EXPECT_EQ(src.key(), 0u);
	EXPECT_EQ(context.renderContext().compiledTexture(dst).id(), srcGlId);
}
