#include "structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp"

#include <utility>

#include <GL/glew.h>

#include "spk_generated_resources.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	DrawColorMeshRenderCommand::DrawColorMeshRenderCommand(
		std::shared_ptr<const spk::ColorMesh2D> p_mesh) :
		_mesh(std::move(p_mesh)),
		_viewportBuffer(spk::Viewport::viewportUniformBuffer())
	{
	}

	spk::Program &DrawColorMeshRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.frag"));
		return program;
	}

	void DrawColorMeshRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (_mesh == nullptr)
		{
			return;
		}

		const spk::LayoutBufferObject &layoutBuffer = _mesh->layoutBuffer();
		if (layoutBuffer.indexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program &program = _sharedProgram().gpu(p_renderContext);

		program.activate();
		layoutBuffer.activate(p_renderContext);

		_viewportBuffer.activate(p_renderContext);

		program.render(spk::Primitive::Triangles, 0, layoutBuffer.indexCount());
	}
}
