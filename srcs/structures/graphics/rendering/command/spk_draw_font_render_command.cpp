#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"

#include <cstdint>
#include <span>
#include <utility>

#include <GL/glew.h>

#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	[[nodiscard]] float outlineThickness(const spk::Font::Size& p_size)
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
	spk::Program& DrawFontRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/font/draw_font.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/font/draw_font.frag"));
		return program;
	}

	DrawFontRenderCommand::DrawFontRenderCommand(
		spk::Font::Atlas& p_atlas,
		std::shared_ptr<const spk::TextureMesh2D> p_mesh,
		spk::Font::Size p_size,
		spk::Color p_color,
		spk::Color p_outlineColor) :
		_atlas(p_atlas),
		_mesh(std::move(p_mesh)),
		_viewportBuffer(spk::Viewport::viewportUniformBuffer()),
		_atlasSampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _sharedProgram()),
		_colorUniform("uColor", _sharedProgram()),
		_outlineColorUniform("uOutlineColor", _sharedProgram()),
		_outlineThicknessUniform("uOutlineThickness", _sharedProgram())
	{
		_layoutBuffer.addAttribute(0, spk::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::LayoutBufferObject::Attribute::Type::Vector2);

		_colorUniform.set(p_color.values());
		_outlineColorUniform.set(p_outlineColor.values());
		_outlineThicknessUniform.set(outlineThickness(p_size));
	}

	void DrawFontRenderCommand::_uploadMesh()
	{
		if (_mesh == nullptr || _layoutBuffer.indexCount() != 0)
		{
			return;
		}

		const spk::TextureMesh2D::Buffer& buffer = _mesh->buffer();
		if (buffer.indexes.empty() == true)
		{
			return;
		}

		_layoutBuffer.setVertices(std::span<const spk::TextureMesh2D::Vertex>(buffer.vertices.data(), buffer.vertices.size()));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>(buffer.indexes.data(), buffer.indexes.size()));
	}

	void DrawFontRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		if (_mesh == nullptr)
		{
			return;
		}

		_uploadMesh();

		if (_layoutBuffer.indexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program& program = _sharedProgram().gpu(p_renderContext);

		_atlas.synchronize();

		program.activate();
		_layoutBuffer.activate(p_renderContext);

		_viewportBuffer.activate(p_renderContext);

		_atlasSampler.bind(_atlas);
		_atlasSampler.activate(p_renderContext);

		// Uniform values are stored per command, but OpenGL keeps uniform state
		// on the shared program, so each draw uploads its own payload.
		_colorUniform.forceActivation();
		_outlineColorUniform.forceActivation();
		_outlineThicknessUniform.forceActivation();

		program.render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
	}
}
