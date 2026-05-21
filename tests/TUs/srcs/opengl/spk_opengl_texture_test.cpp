#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl_wrapper_test_utils.hpp"
#include "opengl/spk_opengl_texture.hpp"

TEST(OpenGLTextureTest, DefaultConstructionProducesInvalidGLId)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::Texture tex;

	EXPECT_EQ(tex.glId(), spk::OpenGL::Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, InheritsFromSpkTexture)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::Texture tex;

	EXPECT_NE(tex.id(), spk::Texture::InvalidID);
}

TEST(OpenGLTextureTest, SynchronizeAfterSetPixelsUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::Texture tex;
	std::vector<uint8_t> pixels(4 * 4 * 4, 255);

	tex.setPixels(pixels, {4, 4}, spk::Texture::Format::RGBA);
	ASSERT_TRUE(tex.needsSynchronization());

	tex.synchronize();

	EXPECT_FALSE(tex.needsSynchronization());
	EXPECT_NE(tex.glId(), spk::OpenGL::Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, SynchronizeCalledTwiceDoesNotReuploadUnlessNewData)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::Texture tex;
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

	spk::OpenGL::Texture src;
	std::vector<uint8_t> pixels(2 * 2 * 4, 200);
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	src.synchronize();
	GLuint originalId = src.glId();
	ASSERT_NE(originalId, spk::OpenGL::Texture::InvalidGLId);

	spk::OpenGL::Texture dst(std::move(src));

	EXPECT_EQ(dst.glId(), originalId);
	EXPECT_EQ(src.glId(), spk::OpenGL::Texture::InvalidGLId);
}

TEST(OpenGLTextureTest, MoveAssignmentPreservesGLId)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::Texture src;
	std::vector<uint8_t> pixels(2 * 2 * 4, 100);
	src.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	src.synchronize();
	GLuint originalId = src.glId();

	spk::OpenGL::Texture dst;
	dst = std::move(src);

	EXPECT_EQ(dst.glId(), originalId);
}

TEST(OpenGLTextureTest, ForceSynchronizationUploadsToGPU)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::Texture tex;
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);
	tex.setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);

	tex.forceSynchronization();

	EXPECT_NE(tex.glId(), spk::OpenGL::Texture::InvalidGLId);
	EXPECT_FALSE(tex.needsSynchronization());
}

#endif
