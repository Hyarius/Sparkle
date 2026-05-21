#include "opengl/spk_opengl_vertex_array_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace
{
	bool hasCurrentOpenGLContext()
	{
#if defined(_WIN32)
		return wglGetCurrentContext() != nullptr;
#else
		return true;
#endif
	}
}

namespace spk::OpenGL
{
	VertexArrayObject::VertexArrayObject()
	{
		requestSynchronization();
	}

	VertexArrayObject::~VertexArrayObject()
	{
		_release();
	}

	void VertexArrayObject::_allocate() const
	{
		if (_id == 0)
		{
			glGenVertexArrays(1, &_id);
		}
	}

	void VertexArrayObject::_release() const
	{
		if (_id != 0 && hasCurrentOpenGLContext() == true)
		{
			glDeleteVertexArrays(1, &_id);
		}
		_id = 0;
	}

	void VertexArrayObject::_synchronize() const
	{
		_allocate();
		glBindVertexArray(_id);

		for (const VertexBufferBinding& binding : _vertexBufferBindings)
		{
			if (binding.buffer == nullptr)
			{
				throw std::runtime_error("spk::OpenGL::VertexArrayObject contains a null vertex buffer binding");
			}

			binding.buffer->activate();
			const Attribute& attribute = binding.attribute;

			glEnableVertexAttribArray(attribute.index);
			glVertexAttribPointer(
				attribute.index,
				attribute.componentCount,
				attribute.componentType,
				attribute.normalized == true ? GL_TRUE : GL_FALSE,
				attribute.stride,
				reinterpret_cast<const void*>(attribute.offset));
		}

		if (_indexBuffer != nullptr)
		{
			_indexBuffer->activate();
		}
	}

	GLuint VertexArrayObject::id() const noexcept
	{
		return _id;
	}

	bool VertexArrayObject::isAllocated() const noexcept
	{
		return _id != 0;
	}

	void VertexArrayObject::addVertexBuffer(std::shared_ptr<VertexBufferObject> p_buffer, Attribute p_attribute)
	{
		if (p_buffer == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::VertexArrayObject::addVertexBuffer requires a valid buffer");
		}

		_vertexBufferBindings.push_back(VertexBufferBinding{
			.buffer = std::move(p_buffer),
			.attribute = p_attribute
		});
		requestSynchronization();
	}

	void VertexArrayObject::clearVertexBuffers()
	{
		_vertexBufferBindings.clear();
		requestSynchronization();
	}

	void VertexArrayObject::setIndexBuffer(std::shared_ptr<IndexBufferObject> p_buffer)
	{
		_indexBuffer = std::move(p_buffer);
		requestSynchronization();
	}

	void VertexArrayObject::clearIndexBuffer()
	{
		_indexBuffer = nullptr;
		requestSynchronization();
	}

	const std::shared_ptr<IndexBufferObject>& VertexArrayObject::indexBuffer() const noexcept
	{
		return _indexBuffer;
	}

	void VertexArrayObject::activate()
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}
		else
		{
			_allocate();
		}

		glBindVertexArray(_id);
	}

	void VertexArrayObject::deactivate() const
	{
		glBindVertexArray(0);
	}
}

#endif
