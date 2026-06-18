#pragma once

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class VertexBufferObject : public BufferObject
	{
	public:
		explicit VertexBufferObject(Usage p_usage = Usage::DynamicDraw, std::size_t p_size = 0);
	};
}
