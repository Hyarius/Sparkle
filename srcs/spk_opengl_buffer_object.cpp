#include "spk_opengl_buffer_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <algorithm>
#include <cstring>
#include <stdexcept>

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

namespace spk::OpenGL
{
	BufferObject::BufferObject(Target p_target, Usage p_usage, std::size_t p_size) :
		_target(p_target),
		_usage(p_usage),
		_cpuBuffer(p_size)
	{
		_resetField();
		requestSynchronization();
	}

	BufferObject::~BufferObject()
	{
		_release();
	}

	void BufferObject::_allocate()
	{
		if (_id == 0)
		{
			glGenBuffers(1, &_id);
		}
	}

	void BufferObject::_release()
	{
		if (_id != 0 && hasCurrentOpenGLContext() == true)
		{
			glDeleteBuffers(1, &_id);
		}
		_id = 0;
		_allocatedSize = 0;
	}

	void BufferObject::_resetField()
	{
		_field = spk::BinaryField(_cpuBuffer.data(), _cpuBuffer.size());
	}

	void BufferObject::_synchronize()
	{
		std::scoped_lock lock(_mutex);
		_allocate();

		const GLenum glTarget = static_cast<GLenum>(_target);
		glBindBuffer(glTarget, _id);

		if (_allocatedSize != _cpuBuffer.size())
		{
			glBufferData(
				glTarget,
				static_cast<GLsizeiptr>(_cpuBuffer.size()),
				_cpuBuffer.empty() == true ? nullptr : _cpuBuffer.data(),
				static_cast<GLenum>(_usage));
			_allocatedSize = _cpuBuffer.size();
		}
		else if (_cpuBuffer.empty() == false)
		{
			glBufferSubData(
				glTarget,
				0,
				static_cast<GLsizeiptr>(_cpuBuffer.size()),
				_cpuBuffer.data());
		}
	}

	GLuint BufferObject::id() const noexcept
	{
		return _id;
	}

	BufferObject::Target BufferObject::target() const noexcept
	{
		return _target;
	}

	BufferObject::Usage BufferObject::usage() const noexcept
	{
		return _usage;
	}

	std::size_t BufferObject::size() const noexcept
	{
		return _cpuBuffer.size();
	}

	bool BufferObject::isAllocated() const noexcept
	{
		return _id != 0;
	}

	void BufferObject::setTarget(Target p_target)
	{
		if (_target == p_target)
		{
			return;
		}

		_target = p_target;
		_allocatedSize = 0;
		requestSynchronization();
	}

	void BufferObject::setUsage(Usage p_usage)
	{
		if (_usage == p_usage)
		{
			return;
		}

		_usage = p_usage;
		_allocatedSize = 0;
		requestSynchronization();
	}

	void BufferObject::resize(std::size_t p_size)
	{
		std::scoped_lock lock(_mutex);
		_cpuBuffer.resize(p_size);
		_resetField();
		requestSynchronization();
	}

	void BufferObject::clear()
	{
		resize(0);
	}

	void BufferObject::edit(const void* p_data, std::size_t p_size, std::size_t p_offset)
	{
		if (p_data == nullptr && p_size != 0)
		{
			throw std::runtime_error("spk::OpenGL::BufferObject::edit received null data with a non-zero size");
		}

		std::scoped_lock lock(_mutex);
		if (p_offset > _cpuBuffer.size() || p_size > _cpuBuffer.size() - p_offset)
		{
			throw std::runtime_error("spk::OpenGL::BufferObject::edit exceeds the CPU buffer bounds");
		}

		if (p_size != 0)
		{
			std::memcpy(_cpuBuffer.data() + p_offset, p_data, p_size);
		}
		requestSynchronization();
	}

	void BufferObject::append(const void* p_data, std::size_t p_size)
	{
		if (p_data == nullptr && p_size != 0)
		{
			throw std::runtime_error("spk::OpenGL::BufferObject::append received null data with a non-zero size");
		}

		std::scoped_lock lock(_mutex);
		const std::size_t offset = _cpuBuffer.size();
		_cpuBuffer.resize(offset + p_size);
		if (p_size != 0)
		{
			std::memcpy(_cpuBuffer.data() + offset, p_data, p_size);
		}
		_resetField();
		requestSynchronization();
	}

	std::uint8_t* BufferObject::data()
	{
		requestSynchronization();
		return _cpuBuffer.data();
	}

	const std::uint8_t* BufferObject::data() const
	{
		return _cpuBuffer.data();
	}

	std::span<std::uint8_t> BufferObject::bytes()
	{
		requestSynchronization();
		return std::span<std::uint8_t>(_cpuBuffer.data(), _cpuBuffer.size());
	}

	std::span<const std::uint8_t> BufferObject::bytes() const
	{
		return std::span<const std::uint8_t>(_cpuBuffer.data(), _cpuBuffer.size());
	}

	spk::BinaryField& BufferObject::field()
	{
		requestSynchronization();
		return _field;
	}

	const spk::BinaryField& BufferObject::field() const
	{
		return _field;
	}

	void BufferObject::bind()
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}
		else
		{
			_allocate();
		}

		glBindBuffer(static_cast<GLenum>(_target), _id);
	}

	void BufferObject::unbind() const
	{
		glBindBuffer(static_cast<GLenum>(_target), 0);
	}

	void BufferObject::bindBase(GLuint p_bindingPoint)
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}
		else
		{
			_allocate();
		}

		glBindBufferBase(static_cast<GLenum>(_target), p_bindingPoint, _id);
	}

	void BufferObject::bindRange(GLuint p_bindingPoint, GLintptr p_offset, GLsizeiptr p_size)
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}
		else
		{
			_allocate();
		}

		glBindBufferRange(static_cast<GLenum>(_target), p_bindingPoint, _id, p_offset, p_size);
	}
}

#endif
