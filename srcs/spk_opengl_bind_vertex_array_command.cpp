#include "spk_opengl_bind_vertex_array_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

namespace spk::OpenGL
{
	BindVertexArrayCommand::BindVertexArrayCommand(std::shared_ptr<spk::OpenGL::VertexArrayObject> p_vertexArray) :
		_vertexArray(std::move(p_vertexArray))
	{
	}

	void BindVertexArrayCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		if (_vertexArray == nullptr)
		{
			glBindVertexArray(0);
			return;
		}

		_vertexArray->bind();
	}
}

#endif
