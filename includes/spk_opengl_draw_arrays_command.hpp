#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

#include "spk_opengl_primitive.hpp"
#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class DrawArraysCommand : public spk::RenderCommand
	{
	private:
		Primitive _primitive = Primitive::Triangles;
		GLint _first = 0;
		GLsizei _count = 0;

	public:
		DrawArraysCommand(Primitive p_primitive, GLint p_first, GLsizei p_count);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
