#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>
#include <optional>

#include <GL/glew.h>

#include "spk_opengl_index_buffer_object.hpp"
#include "spk_opengl_primitive.hpp"
#include "spk_render_command.hpp"

namespace spk::OpenGL
{
	class DrawElementsInstancedCommand : public spk::RenderCommand
	{
	private:
		Primitive _primitive = Primitive::Triangles;
		std::shared_ptr<spk::OpenGL::IndexBufferObject> _indexBuffer = nullptr;
		GLsizei _instanceCount = 0;
		std::optional<GLsizei> _count;
		std::size_t _offset = 0;

	public:
		DrawElementsInstancedCommand(
			Primitive p_primitive,
			std::shared_ptr<spk::OpenGL::IndexBufferObject> p_indexBuffer,
			GLsizei p_instanceCount,
			std::optional<GLsizei> p_count = std::nullopt,
			std::size_t p_offset = 0);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
