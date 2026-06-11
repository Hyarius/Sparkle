#pragma once

#include <GL/glew.h>

#include "structures/graphics/spk_primitive.hpp"

namespace spk::OpenGL
{
	[[nodiscard]] GLenum primitiveType(spk::Primitive p_primitive);
}
