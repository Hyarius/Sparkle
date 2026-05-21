#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include <GL/glew.h>

#include "opengl/spk_opengl_vertex_array_object.hpp"
#include "opengl/spk_opengl_primitive.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "rendering/spk_render_command.hpp"

namespace spk::OpenGL
{
	class DrawArraysCommand : public spk::RenderCommand
	{
	private:
		Primitive _primitive = Primitive::Triangles;
		std::shared_ptr<spk::Program> _program = nullptr;
		std::shared_ptr<spk::OpenGL::VertexArrayObject> _vertexArray = nullptr;
		GLint _first = 0;
		GLsizei _count = 0;

	public:
		DrawArraysCommand(Primitive p_primitive, GLint p_first, GLsizei p_count);
		DrawArraysCommand(
			Primitive p_primitive,
			std::shared_ptr<spk::Program> p_program,
			std::shared_ptr<spk::OpenGL::VertexArrayObject> p_vertexArray,
			GLint p_first,
			GLsizei p_count);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
