#pragma once

#include <cstddef>
#include <string>

#include <GL/glew.h>

namespace spk::OpenGL
{
	class Uniform
	{
	public:
		[[nodiscard]] static GLint findLocation(GLuint p_programId, const std::string &p_name);

		static void validateDeclaration(
			GLuint p_programId,
			const std::string &p_name,
			GLenum p_expectedType,
			const char *p_expectedTypeName,
			std::size_t p_expectedCount);
	};
}
