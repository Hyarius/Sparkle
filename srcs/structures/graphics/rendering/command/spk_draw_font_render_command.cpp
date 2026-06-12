#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include <GL/glew.h>

#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	[[nodiscard]] spk::Vector3 toPosition(const spk::Vector2Int& p_pixel, float p_depth)
	{
		return {
			static_cast<float>(p_pixel.x),
			static_cast<float>(p_pixel.y),
			p_depth
		};
	}

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

	// The program is static, so the uniforms and sampler can be shared by every
	// font command: locations are resolved and validated once per context instead
	// of once per command instance. execute() re-uploads its own values before
	// each draw (single render thread).
	spk::Vector4Uniform& DrawFontRenderCommand::_colorUniform()
	{
		static spk::Vector4Uniform uniform("uColor", _sharedProgram());
		return uniform;
	}

	spk::Vector4Uniform& DrawFontRenderCommand::_outlineColorUniform()
	{
		static spk::Vector4Uniform uniform("uOutlineColor", _sharedProgram());
		return uniform;
	}

	spk::FloatUniform& DrawFontRenderCommand::_outlineThicknessUniform()
	{
		static spk::FloatUniform uniform("uOutlineThickness", _sharedProgram());
		return uniform;
	}

	spk::SamplerObject& DrawFontRenderCommand::_atlasSampler()
	{
		static spk::SamplerObject sampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _sharedProgram());
		return sampler;
	}

	DrawFontRenderCommand::DrawFontRenderCommand(
		spk::Font& p_font,
		spk::Font::Text p_text,
		spk::Vector2Int p_baselinePosition,
		spk::Font::Size p_size,
		spk::Color p_color,
		spk::Color p_outlineColor,
		float p_depth) :
		_atlas(p_font.atlas(p_size)),
		_color(p_color),
		_outlineColor(p_outlineColor),
		_outlineThickness(outlineThickness(p_size)),
		_viewportBuffer(spk::Viewport::viewportUniformBuffer())
	{
		_layoutBuffer.addAttribute(0, spk::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::LayoutBufferObject::Attribute::Type::Vector2);

		_atlas.loadGlyphs(p_text);

		int cursorX = p_baselinePosition.x;

		for (spk::Font::Codepoint character : p_text)
		{
			const spk::Font::Glyph& glyph = _atlas.glyph(character);

			if (glyph.size.x != 0 && glyph.size.y != 0)
			{
				std::array<spk::TextureVertex2D, 4> vertices;

				for (std::size_t i = 0; i < vertices.size(); ++i)
				{
					const spk::Vector2Int pixelPosition = {
						cursorX + glyph.positions[i].x,
						p_baselinePosition.y + glyph.positions[i].y
					};

					vertices[i] = {
						toPosition(pixelPosition, p_depth),
						glyph.uvs[i]
					};
				}

				_mesh.addShape(vertices[0], vertices[1], vertices[3], vertices[2]);
			}

			cursorX += glyph.step.x;
		}
	}

	DrawFontRenderCommand::DrawFontRenderCommand(
		spk::Font& p_font,
		std::string_view p_text,
		spk::Vector2Int p_baselinePosition,
		spk::Font::Size p_size,
		spk::Color p_color,
		spk::Color p_outlineColor,
		float p_depth) :
		DrawFontRenderCommand(
			p_font,
			spk::Font::textFromUTF8(p_text),
			p_baselinePosition,
			p_size,
			p_color,
			p_outlineColor,
			p_depth)
	{
	}

	void DrawFontRenderCommand::_uploadMesh()
	{
		const spk::TextureMesh2D::Buffer& buffer = _mesh.buffer();
		if (_layoutBuffer.indexCount() != 0 || buffer.indexes.empty() == true)
		{
			return;
		}

		_layoutBuffer.setVertices(std::span<const spk::TextureMesh2D::Vertex>(buffer.vertices.data(), buffer.vertices.size()));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>(buffer.indexes.data(), buffer.indexes.size()));

		// The layout buffer keeps its own CPU copy as the per-context upload
		// source, so the mesh copy is dead weight from here on.
		_mesh = spk::TextureMesh2D();
	}

	void DrawFontRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
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

		spk::SamplerObject& sampler = _atlasSampler();
		sampler.bind(_atlas);
		sampler.activate(p_renderContext);

		// The program and uniforms are shared between every font command: this
		// instance's values must be re-uploaded on every draw, not only when
		// they change.
		_colorUniform().set(_color.values());
		_colorUniform().forceActivation();

		_outlineColorUniform().set(_outlineColor.values());
		_outlineColorUniform().forceActivation();

		_outlineThicknessUniform().set(_outlineThickness);
		_outlineThicknessUniform().forceActivation();

		program.render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
	}
}
