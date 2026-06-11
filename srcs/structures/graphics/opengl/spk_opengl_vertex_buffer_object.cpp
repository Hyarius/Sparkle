#include "structures/graphics/spk_vertex_buffer_object.hpp"

namespace spk
{
	VertexBufferObject::VertexBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::Array, p_usage, p_size)
	{
	}
}
