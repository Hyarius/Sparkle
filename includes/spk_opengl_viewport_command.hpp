#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "spk_rect_2d.hpp"
#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class ViewportCommand : public spk::RenderCommand
	{
	private:
		spk::Rect2D _rect;

	public:
		explicit ViewportCommand(const spk::Rect2D& p_rect);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
