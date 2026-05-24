#include "opengl/spk_opengl_draw_arrays_command.hpp"

#include <utility>

namespace spk
{
	DrawArraysCommand::DrawArraysCommand(Primitive p_primitive, GLint p_first, GLsizei p_count) :
		_primitive(p_primitive),
		_first(p_first),
		_count(p_count)
	{
	}

	DrawArraysCommand::DrawArraysCommand(
		Primitive p_primitive,
		std::shared_ptr<spk::Program> p_program,
		std::shared_ptr<spk::VertexArrayObject> p_vertexArray,
		GLint p_first,
		GLsizei p_count) :
		_primitive(p_primitive),
		_program(std::move(p_program)),
		_vertexArray(std::move(p_vertexArray)),
		_first(p_first),
		_count(p_count)
	{
	}

	void DrawArraysCommand::execute(spk::RenderContext& p_renderContext)
	{
		(void)p_renderContext;

		if (_program != nullptr)
		{
			_program->activate();
		}
		if (_vertexArray != nullptr)
		{
			_vertexArray->activate();
		}

		glDrawArrays(static_cast<GLenum>(_primitive), _first, _count);
	}
}
