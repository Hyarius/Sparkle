#include "rendering/render_command/spk_draw_font_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <GL/glew.h>

namespace
{
	const char* vertexShaderSource()
	{
		return
			"#version 330 core\n"
			"layout(location = 0) in vec3 inPosition;\n"
			"layout(location = 1) in vec2 inUV;\n"
			"out vec2 vertexUV;\n"
			"void main()\n"
			"{\n"
			"	vertexUV = inUV;\n"
			"	gl_Position = vec4(inPosition, 1.0);\n"
			"}\n";
	}

	const char* fragmentShaderSource()
	{
		return
			"#version 330 core\n"
			"uniform sampler2D uTexture;\n"
			"uniform vec4 uColor;\n"
			"in vec2 vertexUV;\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	float coverage = texture(uTexture, vertexUV).r;\n"
			"	outColor = vec4(uColor.rgb, uColor.a * coverage);\n"
			"}\n";
	}

	[[nodiscard]] spk::Vector2UInt currentViewportSize()
	{
		std::array<GLint, 4> viewport{};
		glGetIntegerv(GL_VIEWPORT, viewport.data());
		return {
			static_cast<unsigned int>(viewport[2]),
			static_cast<unsigned int>(viewport[3])
		};
	}

	[[nodiscard]] spk::Vector3 toClipSpace(
		const spk::Vector2Int& p_pixel,
		const spk::Vector2UInt& p_viewportSize,
		float p_depth)
	{
		const float x = (2.0f * static_cast<float>(p_pixel.x) / static_cast<float>(p_viewportSize.x)) - 1.0f;
		const float y = 1.0f - (2.0f * static_cast<float>(p_pixel.y) / static_cast<float>(p_viewportSize.y));
		return {x, y, p_depth};
	}
}

namespace spk
{
	DrawFontRenderCommand::DrawFontRenderCommand(
		const spk::Font& p_font,
		std::wstring p_text,
		spk::Vector2Int p_baselinePosition,
		spk::Font::Size p_size,
		spk::Color p_color,
		float p_depth) :
		_font(p_font),
		_text(std::move(p_text)),
		_baselinePosition(p_baselinePosition),
		_size(p_size),
		_color(p_color),
		_depth(p_depth)
	{
		_layoutBuffer.addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2);
	}

	void DrawFontRenderCommand::_ensureProgram() const
	{
		if (_program == nullptr)
		{
			_program = std::make_shared<spk::Program>(vertexShaderSource(), fragmentShaderSource());
			_textureUniformLocation = glGetUniformLocation(_program->id(), "uTexture");
			_colorUniformLocation = glGetUniformLocation(_program->id(), "uColor");
		}
	}

	void DrawFontRenderCommand::_uploadMesh(const spk::Vector2UInt& p_viewportSize, spk::Font::Atlas& p_atlas) const
	{
		if (_cachedViewportSize == p_viewportSize && _layoutBuffer.vertexCount() != 0)
		{
			return;
		}

		_cachedViewportSize = p_viewportSize;

		spk::TextureMesh2D mesh;
		int cursorX = _baselinePosition.x;
		for (wchar_t character : _text)
		{
			const spk::Font::Glyph& glyph = p_atlas.glyph(character);
			if (glyph.size.x != 0 && glyph.size.y != 0)
			{
				std::array<spk::TextureVertex2D, 4> vertices;
				for (std::size_t i = 0; i < vertices.size(); ++i)
				{
					const spk::Vector2Int pixelPosition = {
						cursorX + glyph.positions[i].x,
						_baselinePosition.y + glyph.positions[i].y
					};
					vertices[i] = {toClipSpace(pixelPosition, p_viewportSize, _depth), glyph.uvs[i]};
				}

				mesh.addShape(vertices[0], vertices[1], vertices[3], vertices[2]);
			}
			cursorX += glyph.step.x;
		}

		std::vector<float> vertices;
		const auto appendVertex = [&vertices](const spk::TextureVertex2D& vertex)
		{
			vertices.push_back(vertex.position.x);
			vertices.push_back(vertex.position.y);
			vertices.push_back(vertex.position.z);
			vertices.push_back(vertex.uv.x);
			vertices.push_back(vertex.uv.y);
		};

		if (mesh.buffer().indexes.empty() == false)
		{
			vertices.reserve(mesh.buffer().indexes.size() * 5);
			for (const std::uint32_t index : mesh.buffer().indexes)
			{
				appendVertex(mesh.buffer().vertices[index]);
			}
		}
		else
		{
			vertices.reserve(mesh.buffer().vertices.size() * 5);
			for (const spk::TextureVertex2D& vertex : mesh.buffer().vertices)
			{
				appendVertex(vertex);
			}
		}

		_layoutBuffer.setVertexBytes(vertices.data(), vertices.size() * sizeof(float));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>());
	}

	void DrawFontRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		const spk::Vector2UInt viewportSize = currentViewportSize();
		if (viewportSize.x == 0 || viewportSize.y == 0)
		{
			throw std::runtime_error("DrawFontRenderCommand requires a valid viewport");
		}

		spk::Font::Atlas& atlas = const_cast<spk::Font&>(_font).atlas(_size);
		_ensureProgram();
		_uploadMesh(viewportSize, atlas);

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		atlas.synchronize();
		_layoutBuffer.activate();
		_program->activate();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas.glId());

		if (_textureUniformLocation >= 0)
		{
			glUniform1i(_textureUniformLocation, 0);
		}
		if (_colorUniformLocation >= 0)
		{
			const std::array<float, 4> color = _color.values();
			glUniform4fv(_colorUniformLocation, 1, color.data());
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (_layoutBuffer.isIndexed() == true)
		{
			_program->render(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			_program->renderRaw(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		_layoutBuffer.deactivate();
		_program->deactivate();
	}
}

#endif
