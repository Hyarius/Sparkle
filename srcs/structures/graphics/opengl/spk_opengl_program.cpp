#include "structures/graphics/opengl/spk_opengl_program.hpp"

#include <stdexcept>
#include <utility>

#if defined(_WIN32)
#include <Windows.h>
#endif

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"

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

	spk::RenderContext& currentRenderContext()
	{
		spk::RenderContext* context = spk::RenderContext::current();
		if (context == nullptr)
		{
			throw std::runtime_error("spk::Program requires a current spk::RenderContext to resolve its compiled program");
		}
		return *context;
	}
}

namespace spk
{
	namespace OpenGL
	{
		Program::Program(const std::string& p_vertexShaderSource, const std::string& p_fragmentShaderSource)
		{
			const GLuint vertexShader = _compileShader(GL_VERTEX_SHADER, p_vertexShaderSource);
			const GLuint fragmentShader = _compileShader(GL_FRAGMENT_SHADER, p_fragmentShaderSource);

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
				throw std::runtime_error("spk::OpenGL::Program link failed: " + log);
			}

			_id = newProgram;
		}

		Program::~Program()
		{
			if (_id != 0 && hasCurrentOpenGLContext() == true)
			{
				glDeleteProgram(_id);
			}
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
				throw std::runtime_error("spk::OpenGL::Program shader compilation failed: " + log);
			}

			return shader;
		}

		GLuint Program::id() const noexcept
		{
			return _id;
		}

		void Program::activate() const
		{
			glUseProgram(_id);
		}

		void Program::deactivate() const
		{
			glUseProgram(0);
		}

		void Program::renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount) const
		{
			activate();
			glDrawArrays(
				static_cast<GLenum>(p_primitive),
				static_cast<GLint>(p_firstVertex),
				static_cast<GLsizei>(p_vertexCount));
		}

		void Program::render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount) const
		{
			activate();
			glDrawElements(
				static_cast<GLenum>(p_primitive),
				static_cast<GLsizei>(p_indexCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<const void*>(static_cast<std::uintptr_t>(p_firstIndex) * sizeof(std::uint32_t)));
		}
	}

	Program::Program(std::string p_vertexShaderSource, std::string p_fragmentShaderSource) :
		_vertexShaderSource(std::move(p_vertexShaderSource)),
		_fragmentShaderSource(std::move(p_fragmentShaderSource)),
		_version(1)
	{
		requestSynchronization();
	}

	void Program::_synchronize() const
	{
		(void)currentRenderContext().compiledProgram(*this);
	}

	std::uint64_t Program::key() const noexcept
	{
		return _key;
	}

	std::uint64_t Program::version() const
	{
		std::scoped_lock lock(_sourceMutex);
		return _version;
	}

	void Program::setSources(std::string p_vertexShaderSource, std::string p_fragmentShaderSource)
	{
		{
			std::scoped_lock lock(_sourceMutex);
			_vertexShaderSource = std::move(p_vertexShaderSource);
			_fragmentShaderSource = std::move(p_fragmentShaderSource);
			++_version;
		}
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

	GLuint Program::id() const
	{
		synchronize();
		return currentRenderContext().compiledProgram(*this).id();
	}

	bool Program::isLinked() const noexcept
	{
		spk::RenderContext* context = spk::RenderContext::current();
		return context != nullptr && context->hasCompiledProgram(*this) == true;
	}

	void Program::activate()
	{
		synchronize();
		currentRenderContext().compiledProgram(*this).activate();
	}

	void Program::deactivate() const
	{
		glUseProgram(0);
	}

	void Program::renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount)
	{
		synchronize();
		currentRenderContext().compiledProgram(*this).renderRaw(p_primitive, p_firstVertex, p_vertexCount);
	}

	void Program::render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount)
	{
		synchronize();
		currentRenderContext().compiledProgram(*this).render(p_primitive, p_firstIndex, p_indexCount);
	}
}
