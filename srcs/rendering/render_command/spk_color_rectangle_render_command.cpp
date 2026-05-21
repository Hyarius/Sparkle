#include "rendering/render_command/spk_color_rectangle_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include <GL/glew.h>

#include "math/spk_vector3.hpp"

namespace
{
	const char* vertexShaderSource()
	{
		return
			"#version 330 core\n"
			"layout(location = 0) in vec3 inPosition;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = vec4(inPosition, 1.0);\n"
			"}\n";
	}

	const char* fragmentShaderSource()
	{
		return
			"#version 330 core\n"
			"uniform vec4 uColor;\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	outColor = uColor;\n"
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
	ColorRectangleRenderCommand::ColorRectangleRenderCommand(
		spk::Rect2D p_rect,
		spk::Color p_color,
		float p_depth) :
		_rect(p_rect),
		_color(p_color),
		_depth(p_depth)
	{
		_layoutBuffer.addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3);
	}

	void ColorRectangleRenderCommand::_ensureProgram() const
	{
		if (_program == nullptr)
		{
			_program = std::make_shared<spk::Program>(vertexShaderSource(), fragmentShaderSource());
			_colorUniformLocation = glGetUniformLocation(_program->id(), "uColor");
		}
	}

	void ColorRectangleRenderCommand::_uploadMesh(const spk::Vector2UInt& p_viewportSize) const
	{
		if (_cachedViewportSize == p_viewportSize && _layoutBuffer.vertexCount() != 0)
		{
			return;
		}

		_cachedViewportSize = p_viewportSize;

		const spk::Vector2Int topLeft = _rect.anchor;
		const spk::Vector2Int bottomLeft = {_rect.left(), _rect.bottom()};
		const spk::Vector2Int bottomRight = {_rect.right(), _rect.bottom()};
		const spk::Vector2Int topRight = {_rect.right(), _rect.top()};

		const std::array<spk::Vector3, 4> positions = {
			toClipSpace(topLeft, p_viewportSize, _depth),
			toClipSpace(bottomLeft, p_viewportSize, _depth),
			toClipSpace(bottomRight, p_viewportSize, _depth),
			toClipSpace(topRight, p_viewportSize, _depth)
		};

		std::vector<float> vertices;
		vertices.reserve(4 * 3);
		for (const spk::Vector3& v : positions)
		{
			vertices.push_back(v.x);
			vertices.push_back(v.y);
			vertices.push_back(v.z);
		}

		const std::array<std::uint32_t, 6> indexes = {0, 1, 2, 0, 2, 3};

		_layoutBuffer.setVertexBytes(vertices.data(), vertices.size() * sizeof(float));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>(indexes.data(), indexes.size()));
	}

	void ColorRectangleRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		const spk::Vector2UInt viewportSize = currentViewportSize();
		if (viewportSize.x == 0 || viewportSize.y == 0)
		{
			throw std::runtime_error("ColorRectangleRenderCommand requires a valid viewport");
		}

		_ensureProgram();
		_uploadMesh(viewportSize);

		_layoutBuffer.activate();
		_program->activate();

		if (_colorUniformLocation >= 0)
		{
			const std::array<float, 4> color = _color.values();
			glUniform4fv(_colorUniformLocation, 1, color.data());
		}

		_program->render(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.indexCount());

		_layoutBuffer.deactivate();
		_program->deactivate();
	}
}

#endif
