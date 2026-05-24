#include "opengl/spk_opengl_draw_elements_command.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	DrawElementsCommand::DrawElementsCommand(
		Primitive p_primitive,
		std::shared_ptr<spk::IndexBufferObject> p_indexBuffer,
		std::optional<GLsizei> p_count,
		std::size_t p_offset) :
		_primitive(p_primitive),
		_indexBuffer(std::move(p_indexBuffer)),
		_count(p_count),
		_offset(p_offset)
	{
	}

	DrawElementsCommand::DrawElementsCommand(
		Primitive p_primitive,
		std::shared_ptr<spk::Program> p_program,
		std::shared_ptr<spk::VertexArrayObject> p_vertexArray,
		std::shared_ptr<spk::IndexBufferObject> p_indexBuffer,
		std::optional<GLsizei> p_count,
		std::size_t p_offset) :
		_primitive(p_primitive),
		_program(std::move(p_program)),
		_vertexArray(std::move(p_vertexArray)),
		_indexBuffer(std::move(p_indexBuffer)),
		_count(p_count),
		_offset(p_offset)
	{
	}

	void DrawElementsCommand::execute(spk::RenderContext& p_renderContext)
	{
		(void)p_renderContext;

		if (_indexBuffer == nullptr)
		{
			throw std::runtime_error("spk::DrawElementsCommand requires a valid index buffer");
		}

		if (_program != nullptr)
		{
			_program->activate();
		}
		if (_vertexArray != nullptr)
		{
			_vertexArray->activate();
		}
		_indexBuffer->activate();

		const GLsizei count = _count.value_or(static_cast<GLsizei>(_indexBuffer->count()));
		glDrawElements(
			static_cast<GLenum>(_primitive),
			count,
			_indexBuffer->elementType(),
			reinterpret_cast<const void*>(_offset));
	}
}
