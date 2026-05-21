#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl/spk_opengl_buffer_object.hpp"

namespace spk::OpenGL
{
	class VertexBufferObject : public BufferObject
	{
	public:
		explicit VertexBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);
	};
}

#endif
