#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"

#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	ClearCommand::ClearCommand(const std::array<float, 4>& p_color, GLbitfield p_mask) :
		_color(p_color),
		_mask(p_mask)
	{
	}

	void ClearCommand::execute(spk::RenderContext& p_renderContext)
	{
		if (p_renderContext.supportsOpenGLCommands() == false)
		{
			return;
		}

		glClearColor(_color[0], _color[1], _color[2], _color[3]);
		glClear(_mask);
	}
}
