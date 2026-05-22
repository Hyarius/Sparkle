#include "rendering/render_command/spk_draw_font_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "math/spk_matrix.hpp"
#include "opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"

namespace
{
	const char* vertexShaderSource()
	{
		return
			"#version 330 core\n"
			"layout(location = 0) in vec3 inPosition;\n"
			"layout(location = 1) in vec2 inUV;\n"
			"layout(std140) uniform ViewportData\n"
			"{\n"
			"	mat4 uProjection;\n"
			"};\n"
			"out vec2 vertexUV;\n"
			"void main()\n"
			"{\n"
			"	vertexUV = inUV;\n"
			"	gl_Position = uProjection * vec4(inPosition, 1.0);\n"
			"}\n";
	}

	const char* fragmentShaderSource()
	{
		return
			"#version 330 core\n"
			"uniform sampler2D uTexture;\n"
			"uniform vec4 uColor;\n"
			"uniform vec4 uOutlineColor;\n"
			"uniform float uOutlineThickness;\n"
			"in vec2 vertexUV;\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	float sdf = texture(uTexture, vertexUV).r;\n"
			"	float fillEdge = 0.5;\n"
			"	float outlineEdge = fillEdge - uOutlineThickness;\n"
			"	float fillAlpha = smoothstep(fillEdge - 0.05, fillEdge + 0.05, sdf);\n"
			"	float outlineAlpha = uOutlineThickness > 0.0\n"
			"		? smoothstep(outlineEdge - 0.05, outlineEdge + 0.05, sdf) * (1.0 - fillAlpha)\n"
			"		: 0.0;\n"
			"	vec4 fill    = vec4(uColor.rgb,        uColor.a        * fillAlpha);\n"
			"	vec4 outline = vec4(uOutlineColor.rgb, uOutlineColor.a * outlineAlpha);\n"
			"	outColor = fill + outline * (1.0 - fill.a);\n"
			"}\n";
	}

	[[nodiscard]] spk::Vector3 toPosition(const spk::Vector2Int& p_pixel, float p_depth)
	{
		return {
			static_cast<float>(p_pixel.x),
			static_cast<float>(p_pixel.y),
			p_depth
		};
	}

	spk::OpenGL::UniformBufferObject& viewportUniformBuffer()
	{
		return spk::OpenGL::GPUDataBufferCenter::getUBO(spk::OpenGL::GPUDataBufferCenter::ViewportBlockName);
	}

	void bindViewportUniformBlock(spk::Program& p_program)
	{
		spk::OpenGL::UniformBufferObject& buffer = viewportUniformBuffer();
		if (buffer.bindingPoint().has_value() == false)
		{
			throw std::runtime_error("DrawFontRenderCommand requires a viewport uniform buffer binding point");
		}

		const GLuint blockIndex = glGetUniformBlockIndex(
			p_program.id(),
			spk::OpenGL::GPUDataBufferCenter::ViewportBlockName.data());
		if (blockIndex == GL_INVALID_INDEX)
		{
			throw std::runtime_error("DrawFontRenderCommand requires a viewport uniform block");
		}

		glUniformBlockBinding(p_program.id(), blockIndex, buffer.bindingPoint().value());
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
		float p_depth,
		spk::Color p_outlineColor,
		float p_outlineThickness) :
		_atlas(const_cast<spk::Font&>(p_font).atlas(p_size)),
		_color(p_color),
		_outlineColor(p_outlineColor),
		_outlineThickness(p_outlineThickness),
		_sampler("uTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0),
		_colorUniform("uColor", _program),
		_outlineColorUniform("uOutlineColor", _program),
		_outlineThicknessUniform("uOutlineThickness", _program)
	{
		_program.setSources(vertexShaderSource(), fragmentShaderSource());
		_layoutBuffer.addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2);

		spk::TextureMesh2D mesh;
		int cursorX = p_baselinePosition.x;
		for (wchar_t character : p_text)
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
					vertices[i] = {toPosition(pixelPosition, p_depth), glyph.uvs[i]};
				}
				mesh.addShape(vertices[0], vertices[1], vertices[3], vertices[2]);
			}
			cursorX += glyph.step.x;
		}

		const auto appendVertex = [this](const spk::TextureVertex2D& vertex)
		{
			_vertexData.push_back(vertex.position.x);
			_vertexData.push_back(vertex.position.y);
			_vertexData.push_back(vertex.position.z);
			_vertexData.push_back(vertex.uv.x);
			_vertexData.push_back(vertex.uv.y);
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

	void DrawFontRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		_uploadMesh();

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		_layoutBuffer.activate();
		bindViewportUniformBlock(_program);
		_program.activate();
		viewportUniformBuffer().activate();

		_sampler.bind(_atlas);
		_sampler.activate();

		_colorUniform.set(_color.values());
		_colorUniform.activate();

		_outlineColorUniform.set(_outlineColor.values());
		_outlineColorUniform.activate();

		_outlineThicknessUniform.set(_outlineThickness);
		_outlineThicknessUniform.activate();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (_layoutBuffer.isIndexed() == true)
		{
			_program.render(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			_program.renderRaw(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		_sampler.deactivate();
		_layoutBuffer.deactivate();
		_program.deactivate();
	}
}

#endif
