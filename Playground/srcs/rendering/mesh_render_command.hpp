#pragma once

#include <GL/glew.h>

#include "geometry/mesh3d.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	class MeshRenderCommand : public spk::RenderCommand
	{
	public:
		MeshRenderCommand(
			const Mesh3D &p_mesh,
			const spk::Texture &p_texture,
			const spk::Matrix4x4 &p_modelTransform,
			const spk::Color &p_tint,
			bool p_translucent);

		void execute(spk::RenderContext &p_renderContext) override;

	private:
		static constexpr GLuint ModelBinding = 2;

		struct ModelData
		{
			spk::Matrix4x4 model;
			spk::Color tint;
		};
		static_assert(sizeof(ModelData) == 80);

		[[nodiscard]] static spk::Program &_program();

		const Mesh3D &_mesh;
		ModelData _modelData;
		bool _translucent;
		spk::UniformBufferObject _modelUBO;
		spk::SamplerObject _sampler;
		spk::BoolUniform _translucentUniform;
	};
}
