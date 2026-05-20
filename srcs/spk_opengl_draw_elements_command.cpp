#include "spk_opengl_draw_elements_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>

namespace spk::OpenGL
{
	DrawElementsCommand::DrawElementsCommand(
		Primitive p_primitive,
		std::shared_ptr<spk::OpenGL::IndexBufferObject> p_indexBuffer,
		std::optional<GLsizei> p_count,
		std::size_t p_offset) :
		_primitive(p_primitive),
		_indexBuffer(std::move(p_indexBuffer)),
		_count(p_count),
		_offset(p_offset)
	{
	}

	void DrawElementsCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		if (_indexBuffer == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::DrawElementsCommand requires a valid index buffer");
		}

		_indexBuffer->bind();

		const GLsizei count = _count.value_or(static_cast<GLsizei>(_indexBuffer->count()));
		glDrawElements(
			static_cast<GLenum>(_primitive),
			count,
			_indexBuffer->elementType(),
			reinterpret_cast<const void*>(_offset));
	}
}

#endif
