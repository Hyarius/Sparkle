#include "structures/graphics/spk_shader_storage_buffer_object.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace spk
{
	ShaderStorageBufferObject::ShaderStorageBufferObject(Usage p_usage, std::size_t p_size, std::size_t p_elementSize, std::size_t p_arrayOffset) :
		BufferObject(Target::ShaderStorage, p_usage, std::max(p_size, p_arrayOffset)),
		_elementSize(p_elementSize),
		_arrayOffset(p_arrayOffset)
	{
		_requireLayout(size());
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(GLuint p_bindingPoint, Usage p_usage, std::size_t p_size, std::size_t p_elementSize, std::size_t p_arrayOffset) :
		ShaderStorageBufferObject(p_usage, p_size, p_elementSize, p_arrayOffset)
	{
		setBindingPoint(p_bindingPoint);
	}

	void ShaderStorageBufferObject::_requireLayout(std::size_t p_totalSize) const
	{
		if (_elementSize == 0)
		{
			return;
		}

		if (p_totalSize < _arrayOffset || (p_totalSize - _arrayOffset) % _elementSize != 0)
		{
			throw std::runtime_error(
				"spk::ShaderStorageBufferObject: total size [" + std::to_string(p_totalSize) +
				"] cannot hold the [" + std::to_string(_arrayOffset) + "] byte header plus whole records of [" +
				std::to_string(_elementSize) + "] bytes");
		}
	}

	void ShaderStorageBufferObject::_validateNewSize(std::size_t p_newSize) const
	{
		_requireLayout(p_newSize);
	}

	void ShaderStorageBufferObject::_validateFixedType(std::size_t p_typeSize) const
	{
		if (p_typeSize != _arrayOffset)
		{
			throw std::runtime_error(
				"spk::ShaderStorageBufferObject: fixed type of size [" + std::to_string(p_typeSize) +
				"] does not match the array offset [" + std::to_string(_arrayOffset) + "]");
		}
	}

	void ShaderStorageBufferObject::_validateElementType(std::size_t p_typeSize) const
	{
		if (_elementSize == 0)
		{
			throw std::runtime_error(
				"spk::ShaderStorageBufferObject: cannot map a record type onto a buffer without a record layout");
		}

		if (p_typeSize != _elementSize)
		{
			throw std::runtime_error(
				"spk::ShaderStorageBufferObject: element type of size [" + std::to_string(p_typeSize) +
				"] does not match the record size [" + std::to_string(_elementSize) + "]");
		}
	}

	std::size_t ShaderStorageBufferObject::elementSize() const noexcept
	{
		return _elementSize;
	}

	std::size_t ShaderStorageBufferObject::arrayOffset() const noexcept
	{
		return _arrayOffset;
	}

	std::size_t ShaderStorageBufferObject::count() const noexcept
	{
		if (_elementSize == 0 || size() <= _arrayOffset)
		{
			return 0;
		}

		return (size() - _arrayOffset) / _elementSize;
	}

	void ShaderStorageBufferObject::clear()
	{
		resize(_arrayOffset);
	}

	void ShaderStorageBufferObject::setBindingPoint(GLuint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	void ShaderStorageBufferObject::clearBindingPoint()
	{
		_bindingPoint = std::nullopt;
	}

	std::optional<GLuint> ShaderStorageBufferObject::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	void ShaderStorageBufferObject::activate(const spk::RenderContext &p_context) const
	{
		BufferObject::activate(p_context);
		if (_bindingPoint.has_value() == true)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint.value(), gpu(p_context).id());
		}
	}
}
