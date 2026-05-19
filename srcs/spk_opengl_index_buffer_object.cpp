#include "spk_opengl_index_buffer_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	IndexBufferObject::IndexBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::ElementArray, p_usage, p_size)
	{
	}

	void IndexBufferObject::setElementType(GLenum p_elementType)
	{
		_elementType = p_elementType;
	}

	GLenum IndexBufferObject::elementType() const noexcept
	{
		return _elementType;
	}

	void IndexBufferObject::setCount(std::size_t p_count)
	{
		_count = p_count;
	}

	std::size_t IndexBufferObject::count() const noexcept
	{
		return _count;
	}
}

#endif
