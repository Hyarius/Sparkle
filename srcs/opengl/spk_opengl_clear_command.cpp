#include "opengl/spk_opengl_clear_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl/spk_opengl_render_context.hpp"

namespace spk::OpenGL
{
	ClearCommand::ClearCommand(const std::array<float, 4>& p_color, GLbitfield p_mask) :
		_color(p_color),
		_mask(p_mask)
	{
	}

	void ClearCommand::execute(spk::IRenderContext& p_renderContext)
	{
#if defined(_WIN32)
		if (dynamic_cast<spk::OpenGL::RenderContext*>(&p_renderContext) == nullptr)
		{
			return;
		}
#else
		(void)p_renderContext;
#endif

		glClearColor(_color[0], _color[1], _color[2], _color[3]);
		glClear(_mask);
	}
}

#endif
