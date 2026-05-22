#include "opengl/spk_opengl_viewport.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>

#include <GL/glew.h>

#include "window/spk_surface_state.hpp"

namespace spk::OpenGL
{
	Viewport::Viewport(const spk::Rect2D& p_geometry) :
		spk::Viewport(p_geometry)
	{
	}

	Viewport::Viewport(const spk::Rect2D& p_geometry, const spk::Rect2D& p_scissor) :
		spk::Viewport(p_geometry, p_scissor)
	{
	}

	void Viewport::_applyToGraphicsContext(const spk::ISurfaceState& p_surfaceState) const
	{
		const spk::Vector2UInt& windowSize = p_surfaceState.rect().size;
		if (windowSize.x == 0 || windowSize.y == 0)
		{
			throw std::runtime_error("spk::OpenGL::Viewport::activate() - window size must be positive");
		}

		const GLint viewportGLY = static_cast<GLint>(windowSize.y)
			- _geometry.y()
			- static_cast<GLint>(_geometry.height());

		glViewport(
			static_cast<GLint>(_geometry.x()),
			viewportGLY,
			static_cast<GLsizei>(_geometry.width()),
			static_cast<GLsizei>(_geometry.height()));

		const GLint scissorGLY = static_cast<GLint>(windowSize.y)
			- _scissor.y()
			- static_cast<GLint>(_scissor.height());

		glEnable(GL_SCISSOR_TEST);
		glScissor(
			static_cast<GLint>(_scissor.x()),
			scissorGLY,
			static_cast<GLsizei>(_scissor.width()),
			static_cast<GLsizei>(_scissor.height()));
	}
}

#endif
