#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>

#include <GL/glew.h>

#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class ClearCommand : public spk::RenderCommand
	{
	private:
		std::array<float, 4> _color;
		GLbitfield _mask = GL_COLOR_BUFFER_BIT;

	public:
		explicit ClearCommand(
			const std::array<float, 4>& p_color = {0.05f, 0.06f, 0.08f, 1.0f},
			GLbitfield p_mask = GL_COLOR_BUFFER_BIT);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
