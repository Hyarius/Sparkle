#include <gtest/gtest.h>


#include "opengl_wrapper_test_utils.hpp"
#include "opengl/spk_opengl_sampler_object.hpp"

using SamplerObject = spk::SamplerObject;
using Texture = spk::Texture;

TEST(OpenGLSamplerObjectTest, DefaultConstructionProducesInvalidBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	SamplerObject sampler;

	EXPECT_EQ(sampler.bindingPoint(), -1);
}

TEST(OpenGLSamplerObjectTest, ConstructorWithParametersSetsNameTypeAndBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	SamplerObject sampler("uTexture", SamplerObject::Type::Texture2D, 0);

	EXPECT_EQ(sampler.bindingPoint(), 0);
	EXPECT_EQ(sampler.type(), SamplerObject::Type::Texture2D);
}

TEST(OpenGLSamplerObjectTest, SetBindingPointUpdatesValue)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	SamplerObject sampler;
	sampler.setBindingPoint(3);

	EXPECT_EQ(sampler.bindingPoint(), 3);
}

TEST(OpenGLSamplerObjectTest, SetTypeUpdatesValue)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	SamplerObject sampler;
	sampler.setType(SamplerObject::Type::Texture1D);

	EXPECT_EQ(sampler.type(), SamplerObject::Type::Texture1D);
}

TEST(OpenGLSamplerObjectTest, AllTypeEnumeratorsAreDistinct)
{
	EXPECT_NE(static_cast<GLenum>(SamplerObject::Type::Texture1D),
	          static_cast<GLenum>(SamplerObject::Type::Texture2D));
	EXPECT_NE(static_cast<GLenum>(SamplerObject::Type::Texture2D),
	          static_cast<GLenum>(SamplerObject::Type::Texture3D));
	EXPECT_NE(static_cast<GLenum>(SamplerObject::Type::Texture3D),
	          static_cast<GLenum>(SamplerObject::Type::TextureCubeMap));
}

TEST(OpenGLSamplerObjectTest, BindAssociatesWeakTexture)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto tex = std::make_shared<Texture>();
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);
	tex->setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	tex->synchronize();

	SamplerObject sampler("uTex", SamplerObject::Type::Texture2D, 0);
	EXPECT_NO_THROW(sampler.bind(*tex));
}

TEST(OpenGLSamplerObjectTest, ActivateAndDeactivateDoNotThrow)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	auto tex = std::make_shared<Texture>();
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);
	tex->setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	tex->synchronize();

	SamplerObject sampler("uTex", SamplerObject::Type::Texture2D, 0);
	sampler.bind(*tex);

	EXPECT_NO_THROW(sampler.activate());
	EXPECT_NO_THROW(sampler.deactivate());
}

TEST(OpenGLSamplerObjectTest, BindingPointUpdatesAfterMultipleSets)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	SamplerObject sampler;
	sampler.setBindingPoint(1);
	sampler.setBindingPoint(5);
	sampler.setBindingPoint(2);

	EXPECT_EQ(sampler.bindingPoint(), 2);
}

