#include "spk_opengl_draw_arrays_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	DrawArraysCommand::DrawArraysCommand(Primitive p_primitive, GLint p_first, GLsizei p_count) :
		_primitive(p_primitive),
		_first(p_first),
		_count(p_count)
	{
	}

	void DrawArraysCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		glDrawArrays(static_cast<GLenum>(_primitive), _first, _count);
	}
}

#endif
