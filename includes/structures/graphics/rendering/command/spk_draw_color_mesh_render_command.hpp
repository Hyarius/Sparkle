#pragma once

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/graphics/geometry/spk_color_mesh_2d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	class DrawColorMeshRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::LayoutBufferObject& _layoutBuffer;
		const spk::UniformBufferObject& _viewportBuffer;

		[[nodiscard]] static spk::Program& _sharedProgram();

	public:
		DrawColorMeshRenderCommand(const spk::ColorMesh2D& p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
