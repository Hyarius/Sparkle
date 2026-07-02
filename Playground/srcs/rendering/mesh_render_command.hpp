#pragma once

#include <GL/glew.h>

#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	// Non-instanced 3D draw command: renders one TextureMesh2D with a model matrix and a
	// texture, reading the camera view-projection from UBO binding 1 (set upstream by a
	// spk::CameraUpdateRenderCommand). Modeled on spk::InstancedSpriteRenderCommand but
	// single-draw. Enables back-face culling so a convex mesh renders correctly without a
	// depth buffer (Step 1); real depth testing lands in Step 2.
	class MeshRenderCommand : public spk::RenderCommand
	{
	public:
		MeshRenderCommand(
			const spk::TextureMesh2D &p_mesh,
			const spk::Texture &p_texture,
			const spk::Matrix4x4 &p_modelTransform);

		void execute(spk::RenderContext &p_renderContext) override;

	private:
		static constexpr GLuint ModelBinding = 2;

		[[nodiscard]] static spk::Program &_program();

		const spk::TextureMesh2D &_mesh;
		spk::Matrix4x4 _modelTransform;
		spk::UniformBufferObject _modelUBO;
		spk::SamplerObject _sampler;
	};
}
