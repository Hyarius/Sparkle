#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/opengl/spk_opengl_framebuffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

TEST(FrameBufferObjectTest, ColorTargetDescriptionIsExplicitColorPlusDepthStencil)
{
	const auto description = spk::FrameBufferObject::colorTarget({64, 48});

	EXPECT_EQ(description.size, spk::Vector2UInt(64, 48));
	ASSERT_TRUE(description.color.has_value());
	EXPECT_EQ(description.color->format, spk::Texture::Format::RGBA);
	ASSERT_TRUE(description.depth.has_value());
	EXPECT_EQ(description.depth->storage, spk::FrameBufferObject::AttachmentStorage::RenderBuffer);
	EXPECT_EQ(description.depth->format, spk::Texture::Format::Depth24Stencil8);
}

TEST(FrameBufferObjectTest, DepthTextureTargetDescriptionHasNoUnusedColorAttachment)
{
	const auto description = spk::FrameBufferObject::depthTextureTarget({2048, 2048});

	EXPECT_FALSE(description.color.has_value());
	ASSERT_TRUE(description.depth.has_value());
	EXPECT_EQ(description.depth->storage, spk::FrameBufferObject::AttachmentStorage::Texture);
	EXPECT_EQ(description.depth->format, spk::Texture::Format::Depth24);
}

TEST(FrameBufferObjectTest, RejectsInvalidAttachmentDescriptions)
{
	spk::FrameBufferObject::Description empty{.size = {1, 1}};
	EXPECT_THROW((void)spk::FrameBufferObject{empty}, std::invalid_argument);

	spk::FrameBufferObject::Description depthInColor{
		.size = {1, 1},
		.color = spk::FrameBufferObject::ColorAttachmentDescription{spk::Texture::Format::Depth24}};
	EXPECT_THROW((void)spk::FrameBufferObject{depthInColor}, std::invalid_argument);

	spk::FrameBufferObject::Description colorInDepth{
		.size = {1, 1},
		.depth = spk::FrameBufferObject::DepthAttachmentDescription{
			.storage = spk::FrameBufferObject::AttachmentStorage::Texture,
			.format = spk::Texture::Format::RGBA}};
	EXPECT_THROW((void)spk::FrameBufferObject{colorInDepth}, std::invalid_argument);

	spk::FrameBufferObject::Description unsupportedDepthStencilTexture{
		.size = {1, 1},
		.depth = spk::FrameBufferObject::DepthAttachmentDescription{
			.storage = spk::FrameBufferObject::AttachmentStorage::Texture,
			.format = spk::Texture::Format::Depth24Stencil8}};
	EXPECT_THROW((void)spk::FrameBufferObject{unsupportedDepthStencilTexture}, std::invalid_argument);
}

TEST(FrameBufferObjectTest, RejectsDimensionsOutsideGLsizeiRange)
{
	spk::FrameBufferObject::Description description = spk::FrameBufferObject::colorTarget({1, 1});
	description.size.x = static_cast<unsigned int>(std::numeric_limits<std::int32_t>::max()) + 1u;

	EXPECT_THROW((void)spk::FrameBufferObject{description}, std::invalid_argument);
}

TEST(FrameBufferObjectTest, DefaultsToEmptyWithDepth)
{
	spk::FrameBufferObject fbo;

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(0, 0));
	EXPECT_TRUE(fbo.hasDepth());
}

TEST(FrameBufferObjectTest, ConstructsWithSizeAndConfiguresViewport)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(64, 48));

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(64, 48));
	EXPECT_EQ(fbo.viewport().geometry(), spk::Rect2D(0, 0, 64, 48));
	EXPECT_EQ(fbo.viewport().scissor(), spk::Rect2D(0, 0, 64, 48));
	EXPECT_EQ(fbo.colorAttachment().size(), spk::Vector2UInt(64, 48));
	EXPECT_EQ(fbo.surfaceState().rect(), spk::Rect2D(0, 0, 64, 48));
}

TEST(FrameBufferObjectTest, CanBeConstructedWithoutDepth)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(16, 16), false);

	EXPECT_FALSE(fbo.hasDepth());
}

TEST(FrameBufferObjectTest, DepthTextureAccessorsDistinguishAttachmentPresenceFromStorage)
{
	spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({32, 32}, spk::Texture::Format::Depth32F));

	EXPECT_FALSE(fbo.hasColorAttachment());
	EXPECT_TRUE(fbo.hasDepthAttachment());
	EXPECT_TRUE(fbo.hasSampleableDepth());
	EXPECT_EQ(fbo.tryColorAttachment(), nullptr);
	EXPECT_EQ(fbo.tryDepthAttachment(), &fbo.depthAttachment());
	EXPECT_EQ(fbo.depthAttachment().format(), spk::Texture::Format::Depth32F);
	EXPECT_THROW((void)fbo.colorAttachment(), std::runtime_error);
}

TEST(FrameBufferObjectTest, RenderbufferDepthHasNoDepthTexture)
{
	spk::FrameBufferObject fbo(spk::FrameBufferObject::colorTarget({32, 32}));

	EXPECT_TRUE(fbo.hasColorAttachment());
	EXPECT_TRUE(fbo.hasDepthAttachment());
	EXPECT_FALSE(fbo.hasSampleableDepth());
	EXPECT_NE(fbo.tryColorAttachment(), nullptr);
	EXPECT_EQ(fbo.tryDepthAttachment(), nullptr);
	EXPECT_THROW((void)fbo.depthAttachment(), std::runtime_error);
}

TEST(FrameBufferObjectTest, DepthRenderTargetStoresMetadataWithoutCpuPixels)
{
	spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({2048, 2048}));
	const spk::Texture &depth = fbo.depthAttachment();

	EXPECT_TRUE(depth.isRenderTarget());
	EXPECT_EQ(depth.contentSource(), spk::Texture::ContentSource::RenderTarget);
	EXPECT_EQ(depth.mipmap(), spk::Texture::Mipmap::Disable);
	EXPECT_EQ(depth.filtering(), spk::Texture::Filtering::Nearest);
	EXPECT_EQ(depth.wrap(), spk::Texture::Wrap::ClampToBorder);
	EXPECT_THROW((void)depth.pixels(), std::runtime_error);
	EXPECT_THROW(depth.saveAsPng("depth-target.png"), std::runtime_error);
}

TEST(FrameBufferObjectTest, CopyAndClonePreserveRenderTargetMetadataContracts)
{
	spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({8, 4}, spk::Texture::Format::Depth32F));
	const spk::Texture shared = fbo.depthAttachment();
	const spk::Texture cloned = fbo.depthAttachment().clone();

	EXPECT_EQ(shared.id(), fbo.depthAttachment().id());
	EXPECT_EQ(shared.contentSource(), spk::Texture::ContentSource::RenderTarget);
	EXPECT_NE(cloned.id(), fbo.depthAttachment().id());
	EXPECT_EQ(cloned.size(), spk::Vector2UInt(8, 4));
	EXPECT_EQ(cloned.format(), spk::Texture::Format::Depth32F);
	EXPECT_EQ(cloned.contentSource(), spk::Texture::ContentSource::RenderTarget);
	EXPECT_THROW((void)cloned.pixels(), std::runtime_error);
}

TEST(FrameBufferObjectTest, ResizeUpdatesStateAndBumpsVersion)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	const std::uint64_t before = fbo.version();

	fbo.resize(spk::Vector2UInt(16, 8));

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(16, 8));
	EXPECT_EQ(fbo.viewport().geometry(), spk::Rect2D(0, 0, 16, 8));
	EXPECT_EQ(fbo.colorAttachment().size(), spk::Vector2UInt(16, 8));
	EXPECT_GT(fbo.version(), before);
}

TEST(FrameBufferObjectTest, ResizeFromZeroUpdatesAllAttachmentVersionsExactlyOnce)
{
	spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({0, 0}));
	const std::uint64_t framebufferVersion = fbo.version();
	const std::uint64_t depthVersion = fbo.depthAttachment().version();

	fbo.resize({32, 16});

	EXPECT_EQ(fbo.description().size, spk::Vector2UInt(32, 16));
	EXPECT_EQ(fbo.version(), framebufferVersion + 1);
	EXPECT_EQ(fbo.depthAttachment().version(), depthVersion + 1);
	EXPECT_EQ(fbo.depthAttachment().size(), spk::Vector2UInt(32, 16));
}

TEST(FrameBufferObjectTest, ResizeToZeroPublishesZeroSizedAttachmentMetadata)
{
	spk::FrameBufferObject fbo(spk::FrameBufferObject::colorTarget({16, 16}));
	const std::uint64_t colorVersion = fbo.colorAttachment().version();

	fbo.resize({0, 0});

	EXPECT_EQ(fbo.size(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(fbo.colorAttachment().size(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(fbo.colorAttachment().version(), colorVersion + 1);
}

TEST(FrameBufferObjectTest, ResizeToSameSizeIsANoOp)
{
	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	const std::uint64_t before = fbo.version();

	fbo.resize(spk::Vector2UInt(32, 32));

	EXPECT_EQ(fbo.version(), before);
}

TEST(FrameBufferObjectTest, BuildsACompleteGpuFramebuffer)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 64, 64));

	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	spk::OpenGL::FrameBufferObject &gpu = fbo.gpu(context.renderContext());

	EXPECT_NE(gpu.id(), 0u);
	EXPECT_TRUE(gpu.isComplete());
	EXPECT_NE(fbo.colorAttachment().gpu(context.renderContext()).id(), 0u);
}

TEST(FrameBufferObjectTest, BuildsCompleteColorOnlyGpuFramebuffer)
{
	sparkle_test::OpenGLTestContext context;
	spk::FrameBufferObject::Description description{
		.size = {16, 16},
		.color = spk::FrameBufferObject::ColorAttachmentDescription{}};
	spk::FrameBufferObject fbo(description);

	EXPECT_TRUE(fbo.gpu(context.renderContext()).isComplete());
}

TEST(FrameBufferObjectTest, BuildsCompleteDepthOnlyGpuFramebuffersWithNoDrawOrReadBuffer)
{
	sparkle_test::OpenGLTestContext context;

	for (const spk::Texture::Format format : {spk::Texture::Format::Depth24, spk::Texture::Format::Depth32F})
	{
		spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({16, 16}, format));
		spk::OpenGL::FrameBufferObject &gpu = fbo.gpu(context.renderContext());
		ASSERT_TRUE(gpu.isComplete());
		gpu.bind();

		GLint drawBuffer = 0;
		GLint readBuffer = 0;
		glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
		glGetIntegerv(GL_READ_BUFFER, &readBuffer);
		EXPECT_EQ(drawBuffer, GL_NONE);
		EXPECT_EQ(readBuffer, GL_NONE);
		spk::OpenGL::FrameBufferObject::bindDefault();
	}
}

TEST(FrameBufferObjectTest, DepthTextureUsesExpectedOpenGLFormatDescriptorAndStorage)
{
	sparkle_test::OpenGLTestContext context;
	const struct Case
	{
		spk::Texture::Format format;
		GLint internalFormat;
		GLenum elementType;
	} cases[] = {
		{spk::Texture::Format::Depth24, GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT},
		{spk::Texture::Format::Depth32F, GL_DEPTH_COMPONENT32F, GL_FLOAT}};

	for (const Case &entry : cases)
	{
		const auto descriptor = spk::OpenGL::Texture::formatDescriptor(entry.format);
		EXPECT_EQ(descriptor.internalFormat, entry.internalFormat);
		EXPECT_EQ(descriptor.externalFormat, GL_DEPTH_COMPONENT);
		EXPECT_EQ(descriptor.elementType, entry.elementType);
		EXPECT_TRUE(descriptor.depth);
		EXPECT_FALSE(descriptor.stencil);

		spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({13, 7}, entry.format));
		const GLuint textureId = fbo.depthAttachment().gpu(context.renderContext()).id();
		glBindTexture(GL_TEXTURE_2D, textureId);
		GLint actualInternalFormat = 0;
		GLint width = 0;
		GLint height = 0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &actualInternalFormat);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
		EXPECT_EQ(actualInternalFormat, entry.internalFormat);
		EXPECT_EQ(width, 13);
		EXPECT_EQ(height, 7);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

TEST(FrameBufferObjectTest, GpuConstructionRestoresFramebufferAndRenderbufferBindings)
{
	sparkle_test::OpenGLTestContext context;
	GLuint previousFramebuffer = 0;
	GLuint previousRenderbuffer = 0;
	glGenFramebuffers(1, &previousFramebuffer);
	glGenRenderbuffers(1, &previousRenderbuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, previousRenderbuffer);

	spk::FrameBufferObject fbo(spk::FrameBufferObject::colorTarget({16, 16}));
	(void)fbo.gpu(context.renderContext());

	GLint framebufferBinding = 0;
	GLint renderbufferBinding = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebufferBinding);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbufferBinding);
	EXPECT_EQ(framebufferBinding, static_cast<GLint>(previousFramebuffer));
	EXPECT_EQ(renderbufferBinding, static_cast<GLint>(previousRenderbuffer));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glDeleteFramebuffers(1, &previousFramebuffer);
	glDeleteRenderbuffers(1, &previousRenderbuffer);
}

TEST(FrameBufferObjectTest, DepthOnlyClearForcesAndRestoresDepthWriteMask)
{
	sparkle_test::OpenGLTestContext context;
	spk::RenderContext &renderContext = context.renderContext();
	spk::FrameBufferObject fbo(spk::FrameBufferObject::depthTextureTarget({16, 16}));
	spk::UseFrameBufferRenderCommand useTarget(&fbo, fbo.viewport());
	useTarget.execute(renderContext);

	while (glGetError() != GL_NO_ERROR)
	{
	}
	glDepthMask(GL_FALSE);
	spk::ClearCommand clear({}, GL_DEPTH_BUFFER_BIT);
	clear.execute(renderContext);

	GLboolean depthMask = GL_TRUE;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
	EXPECT_EQ(depthMask, GL_FALSE);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);

	float depth = 0.0f;
	glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	EXPECT_FLOAT_EQ(depth, 1.0f);

	glDepthMask(GL_TRUE);
	spk::OpenGL::FrameBufferObject::bindDefault();
}

TEST(FrameBufferObjectTest, RebuildsACompleteGpuFramebufferAfterResize)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 64, 64));

	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	EXPECT_TRUE(fbo.gpu(context.renderContext()).isComplete());

	fbo.resize(spk::Vector2UInt(48, 24));

	EXPECT_TRUE(fbo.gpu(context.renderContext()).isComplete());
}

TEST(FrameBufferObjectTest, HasGpuOnlyAfterResolution)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 64, 64));

	spk::FrameBufferObject fbo(spk::Vector2UInt(32, 32));
	EXPECT_FALSE(fbo.hasGpu(context.renderContext()));

	(void)fbo.gpu(context.renderContext());
	EXPECT_TRUE(fbo.hasGpu(context.renderContext()));
}
