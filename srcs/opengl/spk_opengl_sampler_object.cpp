#include "opengl/spk_opengl_sampler_object.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk::OpenGL
{
	SamplerObject::SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint) :
		_designator(p_name),
		_bindingPoint(p_bindingPoint),
		_type(p_type)
	{
	}

	void SamplerObject::bind(std::weak_ptr<const OpenGL::Texture> p_texture)
	{
		_texture = std::move(p_texture);
	}

	SamplerObject::BindingPoint SamplerObject::bindingPoint() const
	{
		return _bindingPoint;
	}

	void SamplerObject::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
		_uniformDestination = -1;
	}

	SamplerObject::Type SamplerObject::type() const
	{
		return _type;
	}

	void SamplerObject::setType(Type p_type)
	{
		_type = p_type;
	}

	void SamplerObject::activate()
	{
		std::shared_ptr<const OpenGL::Texture> texture = _texture.lock();

		glActiveTexture(GL_TEXTURE0 + _bindingPoint);

		if (texture == nullptr)
		{
			glBindTexture(static_cast<GLenum>(_type), 0);
			return;
		}

		if (_uniformDestination == -1)
		{
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			_uniformDestination = glGetUniformLocation(prog, _designator.c_str());
			glUniform1i(_uniformDestination, _bindingPoint);
		}

		// Synchronize GPU data if the texture has pending changes
		texture->synchronize();

		glBindTexture(static_cast<GLenum>(_type), texture->glId());
	}

	void SamplerObject::deactivate()
	{
		glActiveTexture(GL_TEXTURE0 + _bindingPoint);
		glBindTexture(static_cast<GLenum>(_type), 0);
	}
}

#endif
