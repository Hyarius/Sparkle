#include "structures/graphics/opengl/spk_opengl_buffer.hpp"

namespace spk
{
	namespace OpenGL
	{
		Buffer::Buffer(GLenum p_target, GLenum p_usage, const void *p_data, std::size_t p_size) :
			_target(p_target),
			_usage(p_usage)
		{
			glGenBuffers(1, &_id);
			upload(p_data, p_size);
		}

		Buffer::~Buffer()
		{
			if (_id != 0 && _ownsCurrentContext() == true)
			{
				glDeleteBuffers(1, &_id);
				notifyBufferDeleted(*this);
			}
		}

		GLuint Buffer::id() const noexcept
		{
			return _id;
		}

		std::size_t Buffer::allocatedSize() const noexcept
		{
			return _allocatedSize;
		}

		void Buffer::upload(const void *p_data, std::size_t p_size)
		{
			glBindBuffer(_target, _id);

			if (_allocatedSize != p_size)
			{
				glBufferData(
					_target,
					static_cast<GLsizeiptr>(p_size),
					p_data,
					_usage);
				_allocatedSize = p_size;
			}
			else if (p_size != 0)
			{
				glBufferSubData(
					_target,
					0,
					static_cast<GLsizeiptr>(p_size),
					p_data);
			}
		}
	}
}
