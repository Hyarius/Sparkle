#include "rendering/render_command/spk_draw_color_mesh_render_command.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	spk::UniformBufferObject& viewportUniformBuffer()
	{
		return spk::GPUDataBufferCenter::getUBO(spk::GPUDataBufferCenter::ViewportBlockName);
	}
}

namespace spk
{
	DrawColorMeshRenderCommand::DrawColorMeshRenderCommand(
		spk::Mesh2D p_mesh,
		spk::Color p_color,
		spk::Matrix3x3 p_transformation) :
		_mesh(std::move(p_mesh)),
		_color(p_color),
		_transformation(p_transformation),
		_layoutBuffer(),
		_layoutBufferDirty(true)
	{
		_layoutBuffer.addAttribute({0, spk::LayoutBufferObject::Attribute::Type::Vector2});
	}

	void DrawColorMeshRenderCommand::_ensureProgram()
	{
		if (_program == nullptr)
		{
			_program = std::make_shared<spk::Program>(
				SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.vert"),
				SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.frag"));
			_colorUniformLocation = glGetUniformLocation(_program->id(), "uColor");
		}
	}

	void DrawColorMeshRenderCommand::_uploadMesh()
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
	}

	void DrawColorMeshRenderCommand::execute(spk::RenderContext& p_renderContext)
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
		viewportUniformBuffer().activate();

		if (_colorUniformLocation >= 0)
		{
			const std::array<float, 4> color = _color.values();
			glUniform4fv(_colorUniformLocation, 1, color.data());
		}

		if (_layoutBuffer.isIndexed() == true)
		{
			_program->render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			_program->renderRaw(spk::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		_layoutBuffer.deactivate();
		_program->deactivate();
	}
}
