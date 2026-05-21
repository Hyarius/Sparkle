#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "math/spk_rect_2d.hpp"
#include "rendering/spk_render_command.hpp"

namespace spk::OpenGL
{
	class ScissorCommand : public spk::RenderCommand
	{
	private:
		spk::Rect2D _rect;
		bool _enabled = true;

	public:
		explicit ScissorCommand(const spk::Rect2D& p_rect, bool p_enabled = true);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
