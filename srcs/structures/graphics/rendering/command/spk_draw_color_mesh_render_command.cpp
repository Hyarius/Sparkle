#include "structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp"

#include <GL/glew.h>

#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "spk_generated_resources.hpp"

namespace spk
{
	DrawColorMeshRenderCommand::DrawColorMeshRenderCommand(
		const spk::ColorMesh2D& p_mesh) :
		_layoutBuffer(p_mesh.layoutBuffer()),
		_viewportBuffer(spk::Viewport::viewportUniformBuffer())
	{
	}

	spk::Program& DrawColorMeshRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/color_mesh/draw_color_mesh.frag"));
		return program;
	}

	void DrawColorMeshRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		if (_layoutBuffer.indexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program& program = _sharedProgram().gpu(p_renderContext);

		program.activate();
		_layoutBuffer.activate(p_renderContext);

		_viewportBuffer.activate(p_renderContext);

		program.render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
	}
}
