#include "structures/graphics/opengl/spk_opengl_primitive.hpp"

#include <stdexcept>

namespace spk::OpenGL
{
	GLenum primitiveType(spk::Primitive p_primitive)
	{
		switch (p_primitive)
		{
		case spk::Primitive::Points:
			return GL_POINTS;
		case spk::Primitive::Lines:
			return GL_LINES;
		case spk::Primitive::LineLoop:
			return GL_LINE_LOOP;
		case spk::Primitive::LineStrip:
			return GL_LINE_STRIP;
		case spk::Primitive::Triangles:
			return GL_TRIANGLES;
		case spk::Primitive::TriangleStrip:
			return GL_TRIANGLE_STRIP;
		case spk::Primitive::TriangleFan:
			return GL_TRIANGLE_FAN;
		}

		throw std::runtime_error("Unsupported primitive type");
	}
}
