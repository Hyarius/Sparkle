#include "structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp"

#include <cstdint>
#include <span>

#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "spk_generated_resources.hpp"

namespace spk
{
	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(
		const spk::Texture& p_texture,
		const spk::TextureMesh2D& p_mesh) :
		_texture(p_texture),
		_mesh(p_mesh),
		_viewportBuffer(spk::Viewport::viewportUniformBuffer()),
		_textureSampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _sharedProgram())
	{
		_textureSampler.bind(_texture);
	}

	spk::Program& DrawTextureMeshRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/texture_mesh/draw_texture_mesh.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/texture_mesh/draw_texture_mesh.frag"));
		return program;
	}

	void DrawTextureMeshRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		if (_mesh.layoutBuffer().indexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program& program = _sharedProgram().gpu(p_renderContext);

		program.activate();
		_mesh.layoutBuffer().activate(p_renderContext);

		_viewportBuffer.activate(p_renderContext);

		_textureSampler.activate(p_renderContext);

		program.render(spk::Primitive::Triangles, 0, _mesh.layoutBuffer().indexCount());
	}
}
