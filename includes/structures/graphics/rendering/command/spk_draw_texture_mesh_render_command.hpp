#pragma once

#include <memory>
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/texture/spk_texture.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/opengl/spk_opengl_layout_buffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"
#include "structures/graphics/opengl/spk_opengl_sampler_object.hpp"

namespace spk
{
	class DrawTextureMeshRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::Texture& _texture;
		spk::TextureMesh2D _mesh;
		spk::SamplerObject _sampler;
		spk::LayoutBufferObject _layoutBuffer;
		bool _layoutBufferDirty;

		[[nodiscard]] static spk::Program& _sharedProgram();
		void _uploadMesh();

	public:
		DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
