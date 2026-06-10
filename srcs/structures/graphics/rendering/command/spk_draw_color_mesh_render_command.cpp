#include "structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "structures/graphics/opengl/spk_opengl_layout_buffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "spk_generated_resources.hpp"

#include <iostream>

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
		spk::ColorMesh2D p_mesh) :
		_mesh(std::move(p_mesh)),
		_layoutBuffer()
	{
		_layoutBuffer.addAttribute({0, spk::LayoutBufferObject::Attribute::Type::Vector3});
		_layoutBuffer.addAttribute({1, spk::LayoutBufferObject::Attribute::Type::Vector4});
	}

	spk::Program& DrawColorMeshRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.frag"));
		return program;
	}

	void DrawColorMeshRenderCommand::_uploadMesh()
	{
		if (_mesh.buffer().vertices.empty() == true)
		{
			return;
		}

		_layoutBuffer.setVertices(std::span<const spk::ColorMesh2D::Vertex>(_mesh.buffer().vertices.data(), _mesh.buffer().vertices.size()));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>(_mesh.buffer().indexes.data(), _mesh.buffer().indexes.size()));
	}

	void DrawColorMeshRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		if (_mesh.buffer().indexes.empty() == true)
		{
			return ;
		}

		spk::OpenGL::Program& program = p_renderContext.compiledProgram(_sharedProgram());

		if (_layoutBuffer.indexCount() == 0)
		{
			_uploadMesh();
		}

		program.activate();
		_layoutBuffer.activate();
		viewportUniformBuffer().activate();

		program.render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());

		_layoutBuffer.deactivate();
		program.deactivate();
	}
}
