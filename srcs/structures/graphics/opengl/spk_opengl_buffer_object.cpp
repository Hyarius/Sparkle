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
		_resetLayout();
		requestSynchronization();
	}

	void BufferObject::_validateNewSize(std::size_t p_newSize) const
	{
		(void)p_newSize;
	}

	void BufferObject::_resetLayout()
	{
		_layout = spk::BinaryLayout(_cpuBuffer.size());
	}

	void BufferObject::_synchronize() const
	{
		spk::RenderContext *ctx = spk::RenderContext::current();
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

	spk::OpenGL::Buffer &BufferObject::gpu(const spk::RenderContext &p_context) const
	{
		std::scoped_lock lock(_mutex);
		return _gpu.resolve(
			p_context,
			_structureVersion,
			_contentVersion,
			[this]() {
				return std::make_unique<spk::OpenGL::Buffer>(
					static_cast<GLenum>(_target),
					static_cast<GLenum>(_usage),
					_cpuBuffer.empty() == true ? nullptr : _cpuBuffer.data(),
					_cpuBuffer.size());
			},
			[this](spk::OpenGL::Buffer &p_buffer) {
				p_buffer.upload(
					_cpuBuffer.empty() == true ? nullptr : _cpuBuffer.data(),
					_cpuBuffer.size());
			});
	}

	bool BufferObject::hasGpu(const spk::RenderContext &p_context) const noexcept
	{
		std::scoped_lock lock(_mutex);
		const spk::OpenGL::Buffer *object = _gpu.find(p_context);
		return object != nullptr &&
			   object->version() == _structureVersion &&
			   object->contentVersion() == _contentVersion;
	}

	void BufferObject::resize(std::size_t p_size)
	{
		_validateNewSize(p_size);

		std::scoped_lock lock(_mutex);
		_cpuBuffer.resize(p_size);
		_resetLayout();
		++_contentVersion;
		requestSynchronization();
	}

	void BufferObject::reserve(std::size_t p_size)
	{
		std::scoped_lock lock(_mutex);
		_cpuBuffer.reserve(p_size);
	}

	void BufferObject::markContentModified()
	{
		std::scoped_lock lock(_mutex);
		++_contentVersion;
		requestSynchronization();
	}

	void BufferObject::clear()
	{
		resize(0);
	}

	void BufferObject::edit(const void *p_data, std::size_t p_size, std::size_t p_offset)
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

	std::size_t BufferObject::_reserveBack(std::size_t p_size, std::size_t p_alignment)
	{
		std::scoped_lock lock(_mutex);

		std::size_t offset = _cpuBuffer.size();
		if (p_alignment > 1)
		{
			const std::size_t remainder = offset % p_alignment;
			if (remainder != 0)
			{
				offset += p_alignment - remainder;
			}
		}

		_validateNewSize(offset + p_size);

		_cpuBuffer.resize(offset + p_size);
		_resetLayout();
		++_contentVersion;
		requestSynchronization();

		return offset;
	}

	std::uint8_t *BufferObject::_addressAt(std::size_t p_offset) noexcept
	{
		return _cpuBuffer.data() + p_offset;
	}

	std::size_t BufferObject::append(const void *p_data, std::size_t p_size)
	{
		if (p_data == nullptr && p_size != 0)
		{
			throw std::runtime_error("spk::BufferObject::append received null data with a non-zero size");
		}

		const std::size_t offset = _reserveBack(p_size, 1);
		if (p_size != 0)
		{
			std::memcpy(_cpuBuffer.data() + offset, p_data, p_size);
		}

		return offset;
	}

	std::uint8_t *BufferObject::data()
	{
		++_contentVersion;
		requestSynchronization();
		return _cpuBuffer.data();
	}

	const std::uint8_t *BufferObject::data() const
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

	spk::BinaryView BufferObject::view()
	{
		++_contentVersion;
		requestSynchronization();
		return spk::BinaryView(std::span<std::byte>(reinterpret_cast<std::byte *>(_cpuBuffer.data()), _cpuBuffer.size()), _layout);
	}

	spk::BinaryView BufferObject::view() const
	{
		return spk::BinaryView(std::span<const std::byte>(reinterpret_cast<const std::byte *>(_cpuBuffer.data()), _cpuBuffer.size()), _layout);
	}

	bool BufferObject::empty() const
	{
		return (_cpuBuffer.empty());
	}

	void BufferObject::activate(const spk::RenderContext &p_context) const
	{
		if (needsSynchronization() == true)
		{
			(void)synchronize();
		}

		glBindBuffer(static_cast<GLenum>(_target), gpu(p_context).id());
	}

	void BufferObject::deactivate() const
	{
		glBindBuffer(static_cast<GLenum>(_target), 0);
	}

	void BufferObject::activateBase(const spk::RenderContext &p_context, GLuint p_bindingPoint) const
	{
		if (needsSynchronization() == true)
		{
			(void)synchronize();
		}

		glBindBufferBase(static_cast<GLenum>(_target), p_bindingPoint, gpu(p_context).id());
	}

	void BufferObject::activateRange(const spk::RenderContext &p_context, GLuint p_bindingPoint, GLintptr p_offset, GLsizeiptr p_size) const
	{
		if (needsSynchronization() == true)
		{
			(void)synchronize();
		}

		glBindBufferRange(static_cast<GLenum>(_target), p_bindingPoint, gpu(p_context).id(), p_offset, p_size);
	}
}
