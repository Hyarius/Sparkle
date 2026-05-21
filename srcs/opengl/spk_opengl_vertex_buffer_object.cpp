#include "opengl/spk_opengl_vertex_buffer_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	VertexBufferObject::VertexBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::Array, p_usage, p_size)
	{
	}
}

#endif
