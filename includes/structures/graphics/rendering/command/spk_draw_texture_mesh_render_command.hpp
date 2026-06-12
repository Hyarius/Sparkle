#pragma once

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	class DrawTextureMeshRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::Texture& _texture;
		spk::TextureMesh2D _mesh;
		spk::LayoutBufferObject _layoutBuffer;
		const spk::UniformBufferObject& _viewportBuffer;

		[[nodiscard]] static spk::Program& _sharedProgram();
		[[nodiscard]] static spk::SamplerObject& _textureSampler();
		void _uploadMesh();

	public:
		DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
