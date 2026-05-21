#include "opengl/spk_opengl_viewport_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

namespace spk::OpenGL
{
	ViewportCommand::ViewportCommand(const spk::Rect2D& p_rect) :
		_rect(p_rect)
	{
	}

	void ViewportCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		glViewport(
			static_cast<GLint>(_rect.x()),
			static_cast<GLint>(_rect.y()),
			static_cast<GLsizei>(_rect.width()),
			static_cast<GLsizei>(_rect.height()));
	}
}

#endif
