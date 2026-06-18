#include "structures/graphics/spk_program.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "structures/graphics/opengl/spk_opengl_primitive.hpp"
#include "structures/graphics/opengl/spk_opengl_render_context.hpp"

namespace
{
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
			if (_id != 0 && _ownsCurrentContext() == true)
			{
				glDeleteProgram(_id);
				notifyProgramDeleted(*this);
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
			spk::RenderContext* context = spk::RenderContext::current();
			if (context != nullptr && context->isProgramActive(this) == true)
			{
				return;
			}

			glUseProgram(_id);

			if (context != nullptr)
			{
				context->setActiveProgram(this);
			}
		}

		void Program::deactivate() const
		{
			spk::RenderContext* context = spk::RenderContext::current();
			if (context != nullptr && context->isProgramActive(nullptr) == true)
			{
				return;
			}

			glUseProgram(0);

			if (context != nullptr)
			{
				context->setActiveProgram(nullptr);
			}
		}

		void Program::renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount) const
		{
			activate();
			glDrawArrays(spk::OpenGL::primitiveType(p_primitive), static_cast<GLint>(p_firstVertex), static_cast<GLsizei>(p_vertexCount));
		}

		void Program::render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount) const
		{
			activate();
			glDrawElements(
				spk::OpenGL::primitiveType(p_primitive),
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
		(void)gpu(currentRenderContext());
	}

	std::uint64_t Program::version() const noexcept
	{
		return _version.load();
	}

	spk::OpenGL::Program& Program::gpu(const spk::RenderContext& p_context) const
	{
		return _gpu.resolve(
			p_context, version(), [this]() { return std::make_unique<spk::OpenGL::Program>(vertexShaderSource(), fragmentShaderSource()); });
	}

	bool Program::hasGpu(const spk::RenderContext& p_context) const noexcept
	{
		const spk::OpenGL::Program* object = _gpu.find(p_context);
		return object != nullptr && object->version() == version();
	}

	void Program::setSources(std::string p_vertexShaderSource, std::string p_fragmentShaderSource)
	{
		{
			std::scoped_lock lock(_sourceMutex);
			_vertexShaderSource = std::move(p_vertexShaderSource);
			_fragmentShaderSource = std::move(p_fragmentShaderSource);
			_version.fetch_add(1);
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
		return gpu(currentRenderContext()).id();
	}

	bool Program::isLinked() const noexcept
	{
		spk::RenderContext* context = spk::RenderContext::current();
		return context != nullptr && hasGpu(*context) == true;
	}

	void Program::activate(const spk::RenderContext& p_context) const
	{
		synchronize();
		gpu(p_context).activate();
	}

	void Program::deactivate() const
	{
		spk::RenderContext* context = spk::RenderContext::current();
		if (context != nullptr && context->isProgramActive(nullptr) == true)
		{
			return;
		}

		glUseProgram(0);

		if (context != nullptr)
		{
			context->setActiveProgram(nullptr);
		}
	}

	void Program::renderRaw(
		const spk::RenderContext& p_context, spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount) const
	{
		synchronize();
		gpu(p_context).renderRaw(p_primitive, p_firstVertex, p_vertexCount);
	}

	void Program::render(const spk::RenderContext& p_context, spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount) const
	{
		synchronize();
		gpu(p_context).render(p_primitive, p_firstIndex, p_indexCount);
	}
}
