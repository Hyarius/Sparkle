#include "opengl/spk_opengl_uniform_buffer_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	UniformBufferObject::UniformBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::Uniform, p_usage, p_size)
	{
	}

	UniformBufferObject::UniformBufferObject(GLuint p_bindingPoint, Usage p_usage, std::size_t p_size) :
		UniformBufferObject(p_usage, p_size)
	{
		setBindingPoint(p_bindingPoint);
	}

	void UniformBufferObject::setBindingPoint(GLuint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	void UniformBufferObject::clearBindingPoint()
	{
		_bindingPoint = std::nullopt;
	}

	std::optional<GLuint> UniformBufferObject::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	void UniformBufferObject::activate()
	{
		BufferObject::activate();
		if (_bindingPoint.has_value() == true)
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint.value(), id());
		}
	}
}

#endif
