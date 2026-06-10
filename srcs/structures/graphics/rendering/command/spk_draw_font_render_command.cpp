#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	spk::UniformBufferObject& viewportUniformBuffer()
	{
		return spk::GPUDataBufferCenter::getUBO(spk::GPUDataBufferCenter::ViewportBlockName);
	}

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

	DrawFontRenderCommand::DrawFontRenderCommand(
		const spk::Font& p_font,
		spk::Font::Text p_text,
		spk::Vector2Int p_baselinePosition,
		spk::Font::Size p_size,
		spk::Color p_color,
		spk::Color p_outlineColor,
		float p_depth) :
		_atlas(const_cast<spk::Font&>(p_font).atlas(p_size)),
		_color(p_color),
		_outlineColor(p_outlineColor),
		_outlineThickness(outlineThickness(p_size)),
		_colorUniform("uColor", _sharedProgram()),
		_outlineColorUniform("uOutlineColor", _sharedProgram()),
		_outlineThicknessUniform("uOutlineThickness", _sharedProgram()),
		_sampler("uTexture", spk::SamplerObject::Type::Texture2D, 0),
		_layoutBufferDirty(true)
	{
		_layoutBuffer.addAttribute(0, spk::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::LayoutBufferObject::Attribute::Type::Vector2);

		_atlas.loadGlyphs(p_text);

		spk::TextureMesh2D mesh;
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

				mesh.addShape(vertices[0], vertices[1], vertices[3], vertices[2]);
			}

			cursorX += glyph.step.x;
		}

		const auto appendVertex = [this](const spk::TextureVertex2D& p_vertex)
		{
			_vertexData.push_back(p_vertex.position.x);
			_vertexData.push_back(p_vertex.position.y);
			_vertexData.push_back(p_vertex.position.z);
			_vertexData.push_back(p_vertex.uv.x);
			_vertexData.push_back(p_vertex.uv.y);
		};

		if (mesh.buffer().indexes.empty() == false)
		{
			_vertexData.reserve(mesh.buffer().indexes.size() * 5);

			for (const std::uint32_t index : mesh.buffer().indexes)
			{
				appendVertex(mesh.buffer().vertices[index]);
			}
		}
		else
		{
			_vertexData.reserve(mesh.buffer().vertices.size() * 5);

			for (const spk::TextureVertex2D& vertex : mesh.buffer().vertices)
			{
				appendVertex(vertex);
			}
		}
	}

	DrawFontRenderCommand::DrawFontRenderCommand(
		const spk::Font& p_font,
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
		if (_layoutBufferDirty == false)
		{
			return;
		}

		_layoutBufferDirty = false;

		_layoutBuffer.setVertexBytes(_vertexData.data(), _vertexData.size() * sizeof(float));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>());
	}

	void DrawFontRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		_uploadMesh();

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program& program = p_renderContext.compiledProgram(_sharedProgram());

		_atlas.synchronize();
		_sampler.bind(_atlas);

		_layoutBuffer.activate();
		program.activate();
		viewportUniformBuffer().activate();

		_sampler.activate();

		// The program is shared between every font command: this instance's values
		// must be re-uploaded on every draw, not only when they change.
		_colorUniform.set(_color.values());
		_colorUniform.forceActivation();

		_outlineColorUniform.set(_outlineColor.values());
		_outlineColorUniform.forceActivation();

		_outlineThicknessUniform.set(_outlineThickness);
		_outlineThicknessUniform.forceActivation();

		if (_layoutBuffer.isIndexed() == true)
		{
			program.render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			program.renderRaw(spk::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		_sampler.deactivate();
		_layoutBuffer.deactivate();
		program.deactivate();
	}
}