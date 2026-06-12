#pragma once

#include <memory>

#include "structures/graphics/geometry/spk_color_mesh_2d.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"

namespace spk
{
	class ColorRectangleRenderCommand : public spk::RenderCommand
	{
	private:
		spk::ColorMesh2D _mesh;
		std::unique_ptr<spk::DrawColorMeshRenderCommand> _colorCommand;

	public:
		ColorRectangleRenderCommand(
			spk::Rect2D p_rect,
			spk::Color p_color,
			float p_depth = 0.0f);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
