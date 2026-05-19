#include "spk_opengl_scissor_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

namespace spk::OpenGL
{
	ScissorCommand::ScissorCommand(const spk::Rect2D& p_rect, bool p_enabled) :
		_rect(p_rect),
		_enabled(p_enabled)
	{
	}

	void ScissorCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		if (_enabled == false)
		{
			glDisable(GL_SCISSOR_TEST);
			return;
		}

		glEnable(GL_SCISSOR_TEST);
		glScissor(
			static_cast<GLint>(_rect.x()),
			static_cast<GLint>(_rect.y()),
			static_cast<GLsizei>(_rect.width()),
			static_cast<GLsizei>(_rect.height()));
	}
}

#endif
