#include "spk_program.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>
#include <utility>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace
{
	bool hasCurrentOpenGLContext()
	{
#if defined(_WIN32)
		return wglGetCurrentContext() != nullptr;
#else
		return true;
#endif
	}
}

namespace spk
{
	Program::Program(std::string p_vertexShaderSource, std::string p_fragmentShaderSource) :
		_vertexShaderSource(std::move(p_vertexShaderSource)),
		_fragmentShaderSource(std::move(p_fragmentShaderSource))
	{
		requestSynchronization();
	}

	Program::~Program()
	{
		_release();
	}

	GLuint Program::_compileShader(GLenum p_type, const std::string& p_source)
	{
		const GLuint shader = glCreateShader(p_type);
		const char* source = p_source.c_str();
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint status = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			GLint logLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
			std::string log(static_cast<std::size_t>(logLength), '\0');
			glGetShaderInfoLog(shader, logLength, nullptr, log.data());
			glDeleteShader(shader);
			throw std::runtime_error("spk::Program shader compilation failed: " + log);
		}

		return shader;
	}

	void Program::_release()
	{
		if (_id != 0 && hasCurrentOpenGLContext() == true)
		{
			glDeleteProgram(_id);
		}
		_id = 0;
	}

	void Program::_synchronize()
	{
		std::string vertexShaderSource;
		std::string fragmentShaderSource;
		{
			std::scoped_lock lock(_sourceMutex);
			vertexShaderSource = _vertexShaderSource;
			fragmentShaderSource = _fragmentShaderSource;
		}

		const GLuint vertexShader = _compileShader(GL_VERTEX_SHADER, vertexShaderSource);
		const GLuint fragmentShader = _compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

		const GLuint newProgram = glCreateProgram();
		glAttachShader(newProgram, vertexShader);
		glAttachShader(newProgram, fragmentShader);
		glLinkProgram(newProgram);

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		GLint status = GL_FALSE;
		glGetProgramiv(newProgram, GL_LINK_STATUS, &status);
		if (status != GL_TRUE)
		{
			GLint logLength = 0;
			glGetProgramiv(newProgram, GL_INFO_LOG_LENGTH, &logLength);
			std::string log(static_cast<std::size_t>(logLength), '\0');
			glGetProgramInfoLog(newProgram, logLength, nullptr, log.data());
			glDeleteProgram(newProgram);
			throw std::runtime_error("spk::Program link failed: " + log);
		}

		_release();
		_id = newProgram;
	}

	GLuint Program::id()
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}
		return _id;
	}

	bool Program::isLinked() const noexcept
	{
		return _id != 0;
	}

	void Program::setSources(std::string p_vertexShaderSource, std::string p_fragmentShaderSource)
	{
		std::scoped_lock lock(_sourceMutex);
		_vertexShaderSource = std::move(p_vertexShaderSource);
		_fragmentShaderSource = std::move(p_fragmentShaderSource);
		requestSynchronization();
	}

	std::string Program::vertexShaderSource() const
	{
		std::scoped_lock lock(_sourceMutex);
		return _vertexShaderSource;
	}

	std::string Program::fragmentShaderSource() const
	{
		std::scoped_lock lock(_sourceMutex);
		return _fragmentShaderSource;
	}

	void Program::bind()
	{
		glUseProgram(id());
	}

	void Program::unbind() const
	{
		glUseProgram(0);
	}
}

#endif
