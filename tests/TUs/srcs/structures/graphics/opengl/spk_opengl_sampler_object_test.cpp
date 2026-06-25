#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/spk_sampler_object.hpp"

using SamplerObject = spk::SamplerObject;
using Texture = spk::Texture;

TEST(OpenGLSamplerObjectTest, ConstructorSetsNameTypeAndBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	auto program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);

	SamplerObject sampler("uTexture", SamplerObject::Type::Texture2D, 0, *program);

	EXPECT_EQ(sampler.bindingPoint(), 0);
	EXPECT_EQ(sampler.type(), SamplerObject::Type::Texture2D);
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

TEST(OpenGLSamplerObjectTest, BindAssociatesTexture)
{
	sparkle_test::OpenGLTestContext context;
	auto program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);

	auto tex = std::make_shared<Texture>();
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);
	tex->setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	tex->synchronize();

	SamplerObject sampler("uTex", SamplerObject::Type::Texture2D, 0, *program);
	EXPECT_NO_THROW(sampler.bind(*tex));
	ASSERT_NE(sampler.texture(), nullptr);
	EXPECT_EQ(sampler.texture()->id(), tex->id());
}

TEST(OpenGLSamplerObjectTest, TextureReturnsNullBeforeBindingAndAfterUnbind)
{
	sparkle_test::OpenGLTestContext context;
	auto program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);

	Texture texture;
	SamplerObject sampler("uTex", SamplerObject::Type::Texture2D, 0, *program);

	EXPECT_EQ(sampler.texture(), nullptr);

	sampler.bind(texture);
	ASSERT_NE(sampler.texture(), nullptr);

	sampler.unbind();

	EXPECT_EQ(sampler.texture(), nullptr);
}

TEST(OpenGLSamplerObjectTest, ActivateAndDeactivateDoNotThrow)
{
	sparkle_test::OpenGLTestContext context;

	auto tex = std::make_shared<Texture>();
	std::vector<uint8_t> pixels(2 * 2 * 4, 255);
	tex->setPixels(pixels, {2, 2}, spk::Texture::Format::RGBA);
	tex->synchronize();

	auto program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	program->activate(context.renderContext());

	SamplerObject sampler("uTex", SamplerObject::Type::Texture2D, 0, *program);
	sampler.bind(*tex);

	EXPECT_NO_THROW(sampler.activate(context.renderContext()));
	EXPECT_NO_THROW(sampler.deactivate());
}

TEST(OpenGLSamplerObjectTest, UnboundActivateAndDeactivateSupportEverySamplerType)
{
	sparkle_test::OpenGLTestContext context;
	auto program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);

	const SamplerObject::Type types[] = {
		SamplerObject::Type::Texture1D,
		SamplerObject::Type::Texture2D,
		SamplerObject::Type::Texture3D,
		SamplerObject::Type::TextureCubeMap
	};

	for (SamplerObject::Type type : types)
	{
		SamplerObject sampler("uTex", type, 0, *program);

		EXPECT_NO_THROW(sampler.activate(context.renderContext()));
		EXPECT_NO_THROW(sampler.deactivate());
	}
}

TEST(OpenGLSamplerObjectTest, ActivateThrowsForUnsupportedSamplerType)
{
	sparkle_test::OpenGLTestContext context;
	auto program = sparkle_test::makeSolidProgram(1.0f, 0.0f, 0.0f);
	SamplerObject sampler("uTex", static_cast<SamplerObject::Type>(0xFFFF), 0, *program);

	EXPECT_THROW(sampler.activate(context.renderContext()), std::runtime_error);
}
