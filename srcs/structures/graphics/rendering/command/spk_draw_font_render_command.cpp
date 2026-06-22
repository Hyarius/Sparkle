#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"

#include <cstdint>
#include <span>

#include <GL/glew.h>

#include "spk_generated_resources.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace
{
	[[nodiscard]] float outlineThickness(const spk::Font::Size &p_size)
	{
		if (p_size.outline == 0)
		{
			return 0.0f;
		}

		const float sdfPadding = static_cast<float>(p_size.outline + 2);
		return static_cast<float>(p_size.outline) * 128.0f / (sdfPadding * 255.0f);
	}
}

namespace spk
{
	spk::Program &DrawFontRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/font/draw_font.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/font/draw_font.frag"));
		return program;
	}

	DrawFontRenderCommand::DrawFontRenderCommand(
		const spk::Font::Atlas &p_atlas,
		const spk::TextureMesh2D &p_mesh,
		spk::Font::Size p_size,
		spk::Color p_color,
		spk::Color p_outlineColor) :
		_atlas(p_atlas),
		_mesh(p_mesh),
		_viewportBuffer(spk::Viewport::viewportUniformBuffer()),
		_atlasSampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _sharedProgram()),
		_colorUniform("uColor", _sharedProgram()),
		_outlineColorUniform("uOutlineColor", _sharedProgram()),
		_outlineThicknessUniform("uOutlineThickness", _sharedProgram())
	{
		_atlasSampler.bind(_atlas);

		_colorUniform.set(p_color.values());
		_outlineColorUniform.set(p_outlineColor.values());
		_outlineThicknessUniform.set(outlineThickness(p_size));
	}

	void DrawFontRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (_mesh.layoutBuffer().indexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program &program = _sharedProgram().gpu(p_renderContext);

		program.activate();
		_mesh.layoutBuffer().activate(p_renderContext);

		_viewportBuffer.activate(p_renderContext);

		_atlasSampler.activate(p_renderContext);

		_colorUniform.activate();
		_outlineColorUniform.activate();
		_outlineThicknessUniform.activate();

		program.render(spk::Primitive::Triangles, 0, _mesh.layoutBuffer().indexCount());
	}
}
