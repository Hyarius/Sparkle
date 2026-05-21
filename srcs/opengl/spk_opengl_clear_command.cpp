#include "opengl/spk_opengl_clear_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	ClearCommand::ClearCommand(const std::array<float, 4>& p_color, GLbitfield p_mask) :
		_color(p_color),
		_mask(p_mask)
	{
	}

	void ClearCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		glClearColor(_color[0], _color[1], _color[2], _color[3]);
		glClear(_mask);
	}
}

#endif
