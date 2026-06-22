#pragma once

#include <memory>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/geometry/spk_color_mesh_2d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk
{
	class DrawColorMeshRenderCommand : public spk::RenderCommand
	{
	private:
		std::shared_ptr<const spk::ColorMesh2D> _mesh;
		const spk::UniformBufferObject &_viewportBuffer;

		[[nodiscard]] static spk::Program &_sharedProgram();

	public:
		DrawColorMeshRenderCommand(std::shared_ptr<const spk::ColorMesh2D> p_mesh);

		void execute(spk::RenderContext &p_renderContext) override;
	};
}
