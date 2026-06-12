#pragma once

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

namespace spk::OpenGL
{
	// Per-context cache of a uniform's resolved location and validation state.
	// No GL resource ownership: the destructor issues no GL calls.
	class UniformLocation : public spk::OpenGL::Object
	{
	public:
		GLint location  = -1;
		bool  validated = false;
	};
}
