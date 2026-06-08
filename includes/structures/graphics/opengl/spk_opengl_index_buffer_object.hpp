#pragma once

#include <cstddef>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_buffer_object.hpp"

namespace spk
{
	class IndexBufferObject : public BufferObject
	{
	private:
		GLenum _elementType = GL_UNSIGNED_INT;
		std::size_t _count = 0;

	public:
		explicit IndexBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		void setElementType(GLenum p_elementType);
		[[nodiscard]] GLenum elementType() const noexcept;

		void setCount(std::size_t p_count);
		[[nodiscard]] std::size_t count() const noexcept;
	};
}
