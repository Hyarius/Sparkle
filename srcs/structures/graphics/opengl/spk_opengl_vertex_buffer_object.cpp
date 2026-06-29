#include "structures/graphics/spk_vertex_buffer_object.hpp"

#include <stdexcept>
#include <string>

namespace spk
{
	VertexBufferObject::VertexBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::Array, p_usage, p_size)
	{
	}

	void VertexBufferObject::_validateCast(std::size_t p_typeSize) const
	{
		if (size() % p_typeSize != 0)
		{
			throw std::runtime_error(
				"spk::VertexBufferObject::cast buffer size [" + std::to_string(size()) +
				"] is not a whole multiple of the vertex size [" + std::to_string(p_typeSize) + "]");
		}
	}
}
