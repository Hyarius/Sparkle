#include "spk_opengl_use_program_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

namespace spk::OpenGL
{
	UseProgramRenderCommand::UseProgramRenderCommand(std::shared_ptr<spk::Program> p_program) :
		_program(std::move(p_program))
	{
	}

	void UseProgramRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		if (_program == nullptr)
		{
			glUseProgram(0);
			return;
		}

		_program->bind();
	}
}

#endif
