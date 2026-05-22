#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "math/spk_rect_2d.hpp"
#include "rendering/render_command/spk_draw_color_mesh_render_command.hpp"
#include "rendering/spk_color.hpp"
#include "rendering/spk_render_command.hpp"

namespace spk
{
	class ColorRectangleRenderCommand : public spk::RenderCommand
	{
	private:
		std::unique_ptr<spk::DrawColorMeshRenderCommand> _colorCommand;

	public:
		ColorRectangleRenderCommand(
			spk::Rect2D p_rect,
			spk::Color p_color,
			float p_depth = 0.0f);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
