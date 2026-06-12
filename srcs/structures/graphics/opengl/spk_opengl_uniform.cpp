#include "structures/graphics/opengl/spk_opengl_uniform.hpp"

#include <stdexcept>

namespace
{
	std::string normalizeUniformName(const std::string& p_name)
	{
		const std::string arraySuffix = "[0]";

		if (p_name.size() >= arraySuffix.size() &&
			p_name.compare(p_name.size() - arraySuffix.size(), arraySuffix.size(), arraySuffix) == 0)
		{
			return p_name.substr(0, p_name.size() - arraySuffix.size());
		}

		return p_name;
	}
}

namespace spk::OpenGL
{
	GLint Uniform::findLocation(GLuint p_programId, const std::string& p_name)
	{
		GLint location = glGetUniformLocation(p_programId, p_name.c_str());

		if (location != -1)
		{
			return location;
		}

		const std::string arraySuffix = "[0]";

		if (p_name.size() >= arraySuffix.size() &&
			p_name.compare(p_name.size() - arraySuffix.size(), arraySuffix.size(), arraySuffix) == 0)
		{
			return -1;
		}

		const std::string arrayElementName = p_name + arraySuffix;
		return glGetUniformLocation(p_programId, arrayElementName.c_str());
	}

	void Uniform::validateDeclaration(
		GLuint p_programId,
		const std::string& p_name,
		GLenum p_expectedType,
		const char* p_expectedTypeName,
		std::size_t p_expectedCount)
	{
		const std::string normalizedName = normalizeUniformName(p_name);

		GLuint uniformIndex = GL_INVALID_INDEX;

		{
			const GLchar* uniformNames[] = {normalizedName.c_str()};
			glGetUniformIndices(p_programId, 1, uniformNames, &uniformIndex);
		}

		if (uniformIndex == GL_INVALID_INDEX)
		{
			const std::string arrayElementName = normalizedName + "[0]";
			const GLchar* uniformNames[] = {arrayElementName.c_str()};
			glGetUniformIndices(p_programId, 1, uniformNames, &uniformIndex);
		}

		if (uniformIndex == GL_INVALID_INDEX)
		{
			throw std::runtime_error(
				"spk::UniformBase: uniform [" + p_name +
				"] not found in active uniform declarations");
		}

		GLint activeType = 0;
		glGetActiveUniformsiv(p_programId, 1, &uniformIndex, GL_UNIFORM_TYPE, &activeType);

		if (static_cast<GLenum>(activeType) != p_expectedType)
		{
			throw std::runtime_error(
				"spk::UniformBase: uniform [" + p_name +
				"] has a different shader type than expected. Expected [" +
				std::string(p_expectedTypeName) + "]");
		}

		GLint activeSize = 0;
		glGetActiveUniformsiv(p_programId, 1, &uniformIndex, GL_UNIFORM_SIZE, &activeSize);

		if (static_cast<std::size_t>(activeSize) < p_expectedCount)
		{
			throw std::runtime_error(
				"UniformBase: uniform array [" + p_name + "] is smaller than requested");
		}
	}
}
