#pragma once

#include <GL/glew.h>

#include <memory>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk
{
	class DrawTextureMesh3DRenderCommand : public spk::RenderCommand
	{
	public:
		struct ModelData
		{
			spk::Matrix4x4 model;
			spk::Color tint;
		};
		static_assert(sizeof(ModelData) == 80);

		DrawTextureMesh3DRenderCommand(
			std::shared_ptr<const spk::TextureMesh3D> p_mesh,
			std::shared_ptr<spk::UniformBufferObject> p_modelUBO,
			std::shared_ptr<spk::SamplerObject> p_sampler,
			bool p_translucent);

		void execute(spk::RenderContext &p_renderContext) override;

		[[nodiscard]] static std::shared_ptr<spk::UniformBufferObject> makeModelUBO(
			const spk::Matrix4x4 &p_model,
			const spk::Color &p_tint);
		static void updateModelUBO(
			spk::UniformBufferObject &p_modelUBO,
			const spk::Matrix4x4 &p_model,
			const spk::Color &p_tint);
		[[nodiscard]] static std::shared_ptr<spk::SamplerObject> makeSampler(const spk::Texture &p_texture);

	private:
		static constexpr GLuint ModelBinding = 2;

		[[nodiscard]] static spk::Program &_program();

		std::shared_ptr<const spk::TextureMesh3D> _mesh;
		std::shared_ptr<spk::UniformBufferObject> _modelUBO;
		std::shared_ptr<spk::SamplerObject> _sampler;
		bool _translucent = false;
	};
}
