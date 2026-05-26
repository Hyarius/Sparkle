#pragma once

#include <memory>
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_sampler_object.hpp"

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
		std::shared_ptr<spk::Program> _program;

		void _ensureProgram();
		void _uploadMesh();

	public:
		DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
