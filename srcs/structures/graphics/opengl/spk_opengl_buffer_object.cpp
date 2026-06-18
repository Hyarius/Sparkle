#include "structures/graphics/spk_buffer_object.hpp"

#include <cstring>
#include <memory>
#include <stdexcept>

#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	BufferObject::BufferObject(Target p_target, Usage p_usage, std::size_t p_size) :
		_target(p_target),
		_usage(p_usage),
		_cpuBuffer(p_size)
	{
		_resetField();
		requestSynchronization();
	}

	void BufferObject::_resetField()
	{
		_field = spk::BinaryField(_cpuBuffer.data(), _cpuBuffer.size());
	}

	void BufferObject::_synchronize() const
	{
		spk::RenderContext* ctx = spk::RenderContext::current();
		if (ctx != nullptr && ctx->supportsOpenGLCommands() == true)
		{
			(void)gpu(*ctx);
		}
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

	std::uint64_t BufferObject::structureVersion() const noexcept
	{
		return _structureVersion;
	}

	std::uint64_t BufferObject::contentVersion() const noexcept
	{
		return _contentVersion;
	}

	spk::OpenGL::Buffer& BufferObject::gpu(const spk::RenderContext& p_context) const
	{
		std::scoped_lock lock(_mutex);
		return _gpu.resolve(
			p_context,
			_structureVersion,
			_contentVersion,
			[this]()
			{
				return std::make_unique<spk::OpenGL::Buffer>(
					static_cast<GLenum>(_target),
					static_cast<GLenum>(_usage),
					_cpuBuffer.empty() == true ? nullptr : _cpuBuffer.data(),
					_cpuBuffer.size());
			},
			[this](spk::OpenGL::Buffer& p_buffer) { p_buffer.upload(_cpuBuffer.empty() == true ? nullptr : _cpuBuffer.data(), _cpuBuffer.size()); });
	}

	bool BufferObject::hasGpu(const spk::RenderContext& p_context) const noexcept
	{
		std::scoped_lock lock(_mutex);
		const spk::OpenGL::Buffer* object = _gpu.find(p_context);
		return object != nullptr && object->version() == _structureVersion && object->contentVersion() == _contentVersion;
	}

	void BufferObject::resize(std::size_t p_size)
	{
		std::scoped_lock lock(_mutex);
		_cpuBuffer.resize(p_size);
		_resetField();
		++_contentVersion;
		requestSynchronization();
	}

	void BufferObject::reserve(std::size_t p_size)
	{
		std::scoped_lock lock(_mutex);
		_cpuBuffer.reserve(p_size);
	}

	void BufferObject::clear()
	{
		resize(0);
	}

	void BufferObject::edit(const void* p_data, std::size_t p_size, std::size_t p_offset)
	{
		if (p_data == nullptr && p_size != 0)
		{
			throw std::runtime_error("spk::BufferObject::edit received null data with a non-zero size");
		}

		std::scoped_lock lock(_mutex);
		if (p_offset > _cpuBuffer.size() || p_size > _cpuBuffer.size() - p_offset)
		{
			throw std::runtime_error("spk::BufferObject::edit exceeds the CPU buffer bounds");
		}

		if (p_size != 0)
		{
			std::memcpy(_cpuBuffer.data() + p_offset, p_data, p_size);
		}
		++_contentVersion;
		requestSynchronization();
	}

	void BufferObject::append(const void* p_data, std::size_t p_size)
	{
		if (p_data == nullptr && p_size != 0)
		{
			throw std::runtime_error("spk::BufferObject::append received null data with a non-zero size");
		}

		std::scoped_lock lock(_mutex);
		const std::size_t offset = _cpuBuffer.size();
		_cpuBuffer.resize(offset + p_size);
		if (p_size != 0)
		{
			std::memcpy(_cpuBuffer.data() + offset, p_data, p_size);
		}
		_resetField();
		++_contentVersion;
		requestSynchronization();
	}

	std::uint8_t* BufferObject::data()
	{
		++_contentVersion;
		requestSynchronization();
		return _cpuBuffer.data();
	}

	const std::uint8_t* BufferObject::data() const
	{
		return _cpuBuffer.data();
	}

	std::span<std::uint8_t> BufferObject::bytes()
	{
		++_contentVersion;
		requestSynchronization();
		return std::span<std::uint8_t>(_cpuBuffer.data(), _cpuBuffer.size());
	}

	std::span<const std::uint8_t> BufferObject::bytes() const
	{
		return std::span<const std::uint8_t>(_cpuBuffer.data(), _cpuBuffer.size());
	}

	spk::BinaryField& BufferObject::field()
	{
		++_contentVersion;
		requestSynchronization();
		return _field;
	}

	const spk::BinaryField& BufferObject::field() const
	{
		return _field;
	}

	bool BufferObject::empty() const
	{
		return (_cpuBuffer.empty());
	}

	void BufferObject::activate(const spk::RenderContext& p_context) const
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}

		glBindBuffer(static_cast<GLenum>(_target), gpu(p_context).id());
	}

	void BufferObject::deactivate() const
	{
		glBindBuffer(static_cast<GLenum>(_target), 0);
	}

	void BufferObject::activateBase(const spk::RenderContext& p_context, GLuint p_bindingPoint) const
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}

		glBindBufferBase(static_cast<GLenum>(_target), p_bindingPoint, gpu(p_context).id());
	}

	void BufferObject::activateRange(const spk::RenderContext& p_context, GLuint p_bindingPoint, GLintptr p_offset, GLsizeiptr p_size) const
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}

		glBindBufferRange(static_cast<GLenum>(_target), p_bindingPoint, gpu(p_context).id(), p_offset, p_size);
	}
}
