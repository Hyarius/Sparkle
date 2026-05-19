#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class DrawArraysCommand : public spk::RenderCommand
	{
	private:
		GLenum _mode = GL_TRIANGLES;
		GLint _first = 0;
		GLsizei _count = 0;

	public:
		DrawArraysCommand(GLenum p_mode, GLint p_first, GLsizei p_count);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
