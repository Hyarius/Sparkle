#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/spk_texture.hpp"

TEST(OpenGLTextureTest, DefaultConstructionProducesNoGPUUpload)
{
	sparkle_test::OpenGLTestContext context;

	spk::Texture tex;

	EXPECT_FALSE(tex.hasGpu(context.renderContext()));
}

TEST(OpenGLTextureTest, HasApplicationTextureID)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::Texture tex;

	EXPECT_NE(tex.id(), spk::Texture::InvalidID);
}

TEST(OpenGLTextureTest, GpuTextureAfterSetPixelsHasValidGLId)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(4 * 4 * 4, 255);

	spk::Texture tex;
	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA);

	spk::OpenGL::Texture& glTex = tex.gpu(context.renderContext());

	EXPECT_NE(glTex.id(), 0u);
	EXPECT_TRUE(tex.hasGpu(context.renderContext()));
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
	EXPECT_TRUE(tex.hasGpu(context.renderContext()));
	EXPECT_NE(tex.gpu(context.renderContext()).id(), 0u);
}

TEST(OpenGLTextureTest, GpuTextureSameVersionReturnsCachedEntry)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 3, 128);

	spk::Texture tex;
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGB);

	const GLuint firstId = tex.gpu(context.renderContext()).id();
	const GLuint secondId = tex.gpu(context.renderContext()).id();

	EXPECT_EQ(firstId, secondId);
}

TEST(OpenGLTextureTest, SetPixelsBumpsVersionAndInvalidatesCache)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 200);

	spk::Texture tex;
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const std::uint64_t firstVersion = tex.gpu(context.renderContext()).version();
	ASSERT_TRUE(tex.hasGpu(context.renderContext()));

	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	EXPECT_FALSE(tex.hasGpu(context.renderContext()));

	spk::OpenGL::Texture& refreshedTexture = tex.gpu(context.renderContext());
	EXPECT_NE(firstVersion, refreshedTexture.version());
	EXPECT_EQ(refreshedTexture.version(), tex.version());
	EXPECT_TRUE(tex.hasGpu(context.renderContext()));
}

TEST(OpenGLTextureTest, CopyProducesIndependentGPUTexture)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 100);

	spk::Texture src;
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const GLuint srcId = src.gpu(context.renderContext()).id();

	spk::Texture dst(src);
	const GLuint dstId = dst.gpu(context.renderContext()).id();

	EXPECT_NE(dst.id(), src.id());
	EXPECT_NE(srcId, dstId);
	EXPECT_NE(dstId, 0u);
}

TEST(OpenGLTextureTest, MoveTransfersGpuCopiesToDestination)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 200);

	spk::Texture src;
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	const GLuint originalId = src.gpu(context.renderContext()).id();
	ASSERT_NE(originalId, 0u);

	spk::Texture dst(std::move(src));

	EXPECT_EQ(dst.gpu(context.renderContext()).id(), originalId);
}

TEST(OpenGLTextureTest, ForceSynchronizationUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);

	spk::Texture tex;
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);

	tex.forceSynchronization();

	EXPECT_FALSE(tex.needsSynchronization());
	EXPECT_NE(tex.gpu(context.renderContext()).id(), 0u);
}

TEST(OpenGLTextureTest, GpuTextureCoversAllSupportedPixelFormats)
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
		{spk::Texture::Format::DualChannel, 2}};

	for (const FormatCase& fc : cases)
	{
		spk::Texture tex;
		std::vector<uint8_t> pixels(fc.bytesPerPixel, 255);
		tex.setPixels(pixels, {1, 1}, fc.format, spk::Texture::Filtering::Linear, spk::Texture::Wrap::ClampToBorder, spk::Texture::Mipmap::Disable);

		EXPECT_NE(tex.gpu(context.renderContext()).id(), 0u);
	}
}

TEST(OpenGLTextureTest, EmptyTextureProducesGLIdZero)
{
	sparkle_test::OpenGLTestContext context;

	spk::Texture empty;
	EXPECT_EQ(empty.gpu(context.renderContext()).id(), 0u);

	spk::Texture invalidFormat;
	invalidFormat.setPixels(std::vector<uint8_t>{255}, {1, 1}, spk::Texture::Format::Error);
	EXPECT_EQ(invalidFormat.gpu(context.renderContext()).id(), 0u);
}

TEST(OpenGLTextureTest, MoveAssignmentTransfersGpuCopies)
{
	sparkle_test::OpenGLTestContext context;

	spk::Texture src;
	src.setPixels(std::vector<uint8_t>(4, 80), {1, 1}, spk::Texture::Format::RGBA);
	const GLuint srcGlId = src.gpu(context.renderContext()).id();

	spk::Texture dst;
	dst.setPixels(std::vector<uint8_t>(4, 160), {1, 1}, spk::Texture::Format::RGBA);
	(void)dst.gpu(context.renderContext());

	dst = std::move(src);

	EXPECT_EQ(dst.gpu(context.renderContext()).id(), srcGlId);
}
