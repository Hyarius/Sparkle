#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>

#include <GL/glew.h>

#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace
{
	void expectStateRestored(bool p_translucent)
	{
		sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
		const auto wasEnabled = [](GLenum p_capability) {
			return glIsEnabled(p_capability) == GL_TRUE;
		};
		const auto setEnabled = [](GLenum p_capability, bool p_enabled) {
			if (p_enabled)
			{
				glEnable(p_capability);
			}
			else
			{
				glDisable(p_capability);
			}
		};
		const bool originalCullFace = wasEnabled(GL_CULL_FACE);
		const bool originalDepthTest = wasEnabled(GL_DEPTH_TEST);
		const bool originalBlend = wasEnabled(GL_BLEND);
		GLint originalCullFaceMode = 0;
		GLint originalFrontFace = 0;
		GLint originalDepthFunc = 0;
		GLint originalBlendSourceRGB = 0;
		GLint originalBlendDestinationRGB = 0;
		GLint originalBlendSourceAlpha = 0;
		GLint originalBlendDestinationAlpha = 0;
		GLboolean originalDepthMask = GL_TRUE;
		glGetIntegerv(GL_CULL_FACE_MODE, &originalCullFaceMode);
		glGetIntegerv(GL_FRONT_FACE, &originalFrontFace);
		glGetIntegerv(GL_DEPTH_FUNC, &originalDepthFunc);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &originalDepthMask);
		glGetIntegerv(GL_BLEND_SRC_RGB, &originalBlendSourceRGB);
		glGetIntegerv(GL_BLEND_DST_RGB, &originalBlendDestinationRGB);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &originalBlendSourceAlpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &originalBlendDestinationAlpha);
		const std::array<std::uint8_t, 4> pixel{255, 255, 255, 255};
		spk::Texture texture;
		texture.setPixels(pixel.data(), {1, 1}, spk::Texture::Format::RGBA);
		auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube());
		auto model = spk::DrawTextureMesh3DRenderCommand::makeModelUBO(
			spk::Matrix4x4::identity(), spk::Color{1.0f, 1.0f, 1.0f, 1.0f});
		auto sampler = spk::DrawTextureMesh3DRenderCommand::makeSampler(texture);

		spk::CameraUpdateRenderCommand camera(1, spk::Matrix4x4::identity());
		camera.execute(context.renderContext());
		spk::DirectionalLightUpdateRenderCommand light(
			3,
			spk::DirectionalLight{
				.direction = {0.0f, 0.0f, -1.0f},
				.color = {1.0f, 1.0f, 1.0f, 1.0f},
				.ambient = 1.0f});
		light.execute(context.renderContext());

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glFrontFace(GL_CW);
		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_GREATER);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_COLOR, GL_DST_COLOR, GL_ONE, GL_ZERO);

		spk::DrawTextureMesh3DRenderCommand command(mesh, model, sampler, p_translucent);
		command.execute(context.renderContext());

		EXPECT_EQ(glIsEnabled(GL_CULL_FACE), GL_FALSE);
		EXPECT_EQ(glIsEnabled(GL_DEPTH_TEST), GL_FALSE);
		EXPECT_EQ(glIsEnabled(GL_BLEND), GL_TRUE);
		GLint value = 0;
		GLboolean mask = GL_TRUE;
		glGetIntegerv(GL_CULL_FACE_MODE, &value);
		EXPECT_EQ(value, GL_FRONT);
		glGetIntegerv(GL_FRONT_FACE, &value);
		EXPECT_EQ(value, GL_CW);
		glGetIntegerv(GL_DEPTH_FUNC, &value);
		EXPECT_EQ(value, GL_GREATER);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
		EXPECT_EQ(mask, GL_FALSE);
		glGetIntegerv(GL_BLEND_SRC_RGB, &value);
		EXPECT_EQ(value, GL_SRC_COLOR);
		glGetIntegerv(GL_BLEND_DST_RGB, &value);
		EXPECT_EQ(value, GL_DST_COLOR);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &value);
		EXPECT_EQ(value, GL_ONE);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &value);
		EXPECT_EQ(value, GL_ZERO);

		setEnabled(GL_CULL_FACE, originalCullFace);
		setEnabled(GL_DEPTH_TEST, originalDepthTest);
		setEnabled(GL_BLEND, originalBlend);
		glCullFace(static_cast<GLenum>(originalCullFaceMode));
		glFrontFace(static_cast<GLenum>(originalFrontFace));
		glDepthFunc(static_cast<GLenum>(originalDepthFunc));
		glDepthMask(originalDepthMask);
		glBlendFuncSeparate(
			static_cast<GLenum>(originalBlendSourceRGB),
			static_cast<GLenum>(originalBlendDestinationRGB),
			static_cast<GLenum>(originalBlendSourceAlpha),
			static_cast<GLenum>(originalBlendDestinationAlpha));
	}
}

TEST(DrawTextureMesh3DRenderCommand, RestoresOpenGLStateAfterOpaqueDraw)
{
	expectStateRestored(false);
}

TEST(DrawTextureMesh3DRenderCommand, RestoresOpenGLStateAfterTranslucentDraw)
{
	expectStateRestored(true);
}
