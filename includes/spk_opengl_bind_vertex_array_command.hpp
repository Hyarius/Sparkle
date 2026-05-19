#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "spk_opengl_vertex_array_object.hpp"
#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class BindVertexArrayCommand : public spk::RenderCommand
	{
	private:
		std::shared_ptr<spk::OpenGL::VertexArrayObject> _vertexArray;

	public:
		explicit BindVertexArrayCommand(std::shared_ptr<spk::OpenGL::VertexArrayObject> p_vertexArray = nullptr);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
