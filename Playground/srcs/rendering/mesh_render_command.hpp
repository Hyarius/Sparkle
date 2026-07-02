#pragma once

#include <GL/glew.h>

#include <memory>

#include "geometry/mesh3d.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace pg
{
	// A single mesh draw. The command holds *shared* references to its GPU resources — the
	// mesh, the per-object model UBO (model matrix + tint), and the texture sampler — which
	// the render logics create once and reuse across frames; execute() allocates nothing.
	//
	// Shared ownership is also what makes rendering thread-safe. Commands are built on the
	// update thread and executed on a separate render thread (see the spk render loop); the
	// published snapshot keeps these shared_ptr resources alive while the render thread draws,
	// even if the game swaps in a new mesh on the update thread the same frame.
	class MeshRenderCommand : public spk::RenderCommand
	{
	public:
		struct ModelData
		{
			spk::Matrix4x4 model;
			spk::Color tint;
		};
		static_assert(sizeof(ModelData) == 80);

		MeshRenderCommand(
			std::shared_ptr<const Mesh3D> p_mesh,
			std::shared_ptr<spk::UniformBufferObject> p_modelUBO,
			std::shared_ptr<spk::SamplerObject> p_sampler,
			bool p_translucent);

		void execute(spk::RenderContext &p_renderContext) override;

		// Factories/mutators for the persistent per-object GPU resources the logics cache.
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

		std::shared_ptr<const Mesh3D> _mesh;
		std::shared_ptr<spk::UniformBufferObject> _modelUBO;
		std::shared_ptr<spk::SamplerObject> _sampler;
		bool _translucent;
	};
}
