#include "spk_opengl_draw_arrays_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	DrawArraysCommand::DrawArraysCommand(GLenum p_mode, GLint p_first, GLsizei p_count) :
		_mode(p_mode),
		_first(p_first),
		_count(p_count)
	{
	}

	void DrawArraysCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		glDrawArrays(_mode, _first, _count);
	}
}

#endif
