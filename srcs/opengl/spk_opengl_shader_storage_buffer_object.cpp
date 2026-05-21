#include "opengl/spk_opengl_shader_storage_buffer_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	ShaderStorageBufferObject::ShaderStorageBufferObject(Usage p_usage, std::size_t p_size) :
		BufferObject(Target::ShaderStorage, p_usage, p_size)
	{
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(GLuint p_bindingPoint, Usage p_usage, std::size_t p_size) :
		ShaderStorageBufferObject(p_usage, p_size)
	{
		setBindingPoint(p_bindingPoint);
	}

	void ShaderStorageBufferObject::setBindingPoint(GLuint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	void ShaderStorageBufferObject::clearBindingPoint()
	{
		_bindingPoint = std::nullopt;
	}

	std::optional<GLuint> ShaderStorageBufferObject::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	void ShaderStorageBufferObject::activate()
	{
		BufferObject::activate();
		if (_bindingPoint.has_value() == true)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint.value(), id());
		}
	}
}

#endif
