#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"

#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	ClearCommand::ClearCommand(const std::array<float, 4> &p_color, GLbitfield p_mask) :
		_color(p_color),
		_mask(p_mask)
	{
	}

	void ClearCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (p_renderContext.supportsOpenGLCommands() == false)
		{
			return;
		}

		GLfloat previousClearColor[4]{};
		GLboolean previousDepthMask = GL_TRUE;
		GLint previousFrontStencilMask = 0;
		GLint previousBackStencilMask = 0;

		if ((_mask & GL_COLOR_BUFFER_BIT) != 0)
		{
			glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
			glClearColor(_color[0], _color[1], _color[2], _color[3]);
		}
		if ((_mask & GL_DEPTH_BUFFER_BIT) != 0)
		{
			glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
			glDepthMask(GL_TRUE);
		}
		if ((_mask & GL_STENCIL_BUFFER_BIT) != 0)
		{
			glGetIntegerv(GL_STENCIL_WRITEMASK, &previousFrontStencilMask);
			glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &previousBackStencilMask);
			glStencilMaskSeparate(GL_FRONT, 0xFFFFFFFFu);
			glStencilMaskSeparate(GL_BACK, 0xFFFFFFFFu);
		}

		glClear(_mask);

		if ((_mask & GL_STENCIL_BUFFER_BIT) != 0)
		{
			glStencilMaskSeparate(GL_FRONT, static_cast<GLuint>(previousFrontStencilMask));
			glStencilMaskSeparate(GL_BACK, static_cast<GLuint>(previousBackStencilMask));
		}
		if ((_mask & GL_DEPTH_BUFFER_BIT) != 0)
		{
			glDepthMask(previousDepthMask);
		}
		if ((_mask & GL_COLOR_BUFFER_BIT) != 0)
		{
			glClearColor(
				previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
		}
	}
}
