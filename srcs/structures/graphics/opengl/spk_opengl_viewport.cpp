#include "structures/graphics/opengl/spk_opengl_viewport.hpp"

#include <stdexcept>

#include <GL/glew.h>

#include "structures/system/device/window/spk_surface_state.hpp"

namespace spk
{
	void OpenGLViewport::apply(const spk::Viewport& p_viewport, const spk::SurfaceState& p_surfaceState)
	{
		const spk::Vector2UInt& windowSize = p_surfaceState.rect().size;
		if (windowSize.x == 0 || windowSize.y == 0)
		{
			throw std::runtime_error("spk::OpenGLViewport::apply() - window size must be positive");
		}

		const spk::Rect2D& geometry = p_viewport.geometry();
		const GLint viewportGLY = static_cast<GLint>(windowSize.y) - geometry.y() - static_cast<GLint>(geometry.height());

		glViewport(static_cast<GLint>(geometry.x()), viewportGLY, static_cast<GLsizei>(geometry.width()), static_cast<GLsizei>(geometry.height()));

		const spk::Rect2D& scissor = p_viewport.scissor();
		const GLint scissorGLY = static_cast<GLint>(windowSize.y) - scissor.y() - static_cast<GLint>(scissor.height());

		glScissor(static_cast<GLint>(scissor.x()), scissorGLY, static_cast<GLsizei>(scissor.width()), static_cast<GLsizei>(scissor.height()));
	}
}
