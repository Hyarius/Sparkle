#pragma once

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

namespace spk::OpenGL
{
	class UniformLocation : public spk::OpenGL::Object
	{
	public:
		GLint location = -1;
		bool validated = false;
	};
}
