#include "rendering/render_command/spk_draw_color_mesh_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "rendering/spk_render_context.hpp"

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
}

namespace spk
{
	DrawColorMeshRenderCommand::DrawColorMeshRenderCommand(spk::Mesh2D p_mesh, spk::Color p_color, spk::Matrix3x3 p_transformation) :
		_mesh(std::move(p_mesh)),
		_color(p_color),
		_transformation(p_transformation)
	{
		_layoutBuffer.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});
	}

	void DrawColorMeshRenderCommand::_ensureProgram() const
	{
		if (_program == nullptr)
		{
			_program = std::make_shared<spk::Program>(vertexShaderSource(), fragmentShaderSource());
			_colorUniformLocation = glGetUniformLocation(_program->id(), "uColor");
		}
	}

	void DrawColorMeshRenderCommand::_uploadMesh() const
	{
		if (_layoutBufferDirty == false)
		{
			return;
		}

		_layoutBufferDirty = false;

		if (_mesh.buffer().vertices.empty() == true)
		{
			return;
		}

		std::vector<spk::Vector2> transformedVertices;
		transformedVertices.reserve(_mesh.buffer().vertices.size());
		for (const spk::Vertex2D& vertex : _mesh.buffer().vertices)
		{
			transformedVertices.push_back(_transformation * vertex.position);
		}

		_layoutBuffer.setVertices(std::span<const spk::Vector2>(transformedVertices.data(), transformedVertices.size()));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>(_mesh.buffer().indexes.data(), _mesh.buffer().indexes.size()));
		_layoutBufferDirty = false;
	}

	void DrawColorMeshRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		_ensureProgram();
		_uploadMesh();

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		_layoutBuffer.activate();
		_program->activate();

		if (_colorUniformLocation >= 0)
		{
			const std::array<float, 4> color = _color.values();
			glUniform4fv(_colorUniformLocation, 1, color.data());
		}

		if (_layoutBuffer.isIndexed() == true)
		{
			_program->render(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			_program->renderRaw(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		_layoutBuffer.deactivate();
		_program->deactivate();
	}
}

#endif
