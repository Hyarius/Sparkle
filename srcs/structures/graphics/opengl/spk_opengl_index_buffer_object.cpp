#include "structures/graphics/spk_index_buffer_object.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace
{
	[[nodiscard]] std::size_t indexElementSize(GLenum p_elementType) noexcept
	{
		switch (p_elementType)
		{
		case GL_UNSIGNED_BYTE:
			return sizeof(std::uint8_t);
		case GL_UNSIGNED_SHORT:
			return sizeof(std::uint16_t);
		case GL_UNSIGNED_INT:
			return sizeof(std::uint32_t);
		default:
			return 0;
		}
	}
}

namespace spk
{
	IndexBufferObject::IndexBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::ElementArray, p_usage, p_size)
	{
	}

	void IndexBufferObject::setElementType(GLenum p_elementType)
	{
		if (indexElementSize(p_elementType) == 0)
		{
			throw std::runtime_error("spk::IndexBufferObject has an unsupported element type [" + std::to_string(p_elementType) + "]");
		}
		_elementType = p_elementType;
	}

	GLenum IndexBufferObject::elementType() const noexcept
	{
		return _elementType;
	}

	std::size_t IndexBufferObject::count() const noexcept
	{
		return size() / indexElementSize(_elementType);
	}

	void IndexBufferObject::_validateCast(std::size_t p_typeSize) const
	{
		const std::size_t expected = indexElementSize(_elementType);
		if (p_typeSize != expected)
		{
			throw std::runtime_error(
				"spk::IndexBufferObject::cast index type of size [" + std::to_string(p_typeSize) +
				"] does not match the configured element size [" + std::to_string(expected) + "]");
		}
	}
}
