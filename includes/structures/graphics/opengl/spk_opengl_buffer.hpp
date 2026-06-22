#pragma once

#include <cstddef>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

namespace spk
{
	namespace OpenGL
	{
		// GPU-side buffer storage. Bound to the RenderContext that allocated it and
		// only usable while that context is current.
		class Buffer : public Object
		{
		private:
			GLuint _id = 0;
			GLenum _target;
			GLenum _usage;
			std::size_t _allocatedSize = 0;

		public:
			Buffer(GLenum p_target, GLenum p_usage, const void *p_data, std::size_t p_size);
			~Buffer() override;

			Buffer(const Buffer &) = delete;
			Buffer &operator=(const Buffer &) = delete;
			Buffer(Buffer &&) = delete;
			Buffer &operator=(Buffer &&) = delete;

			[[nodiscard]] GLuint id() const noexcept;
			[[nodiscard]] std::size_t allocatedSize() const noexcept;

			// Binds the buffer and uploads p_data: in place (glBufferSubData) when the
			// size is unchanged, reallocating (glBufferData) otherwise.
			void upload(const void *p_data, std::size_t p_size);
		};
	}
}
