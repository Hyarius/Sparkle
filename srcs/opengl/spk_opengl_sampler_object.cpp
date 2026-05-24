#include "opengl/spk_opengl_sampler_object.hpp"

namespace spk
{
	SamplerObject::SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint) :
		_designator(p_name),
		_bindingPoint(p_bindingPoint),
		_type(p_type)
	{
	}

	void SamplerObject::bind(const spk::GPUTexture& p_texture)
	{
		_texture = &p_texture;
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
		glActiveTexture(GL_TEXTURE0 + _bindingPoint);

		if (_texture == nullptr)
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

		_texture->synchronize();

		glBindTexture(static_cast<GLenum>(_type), _texture->glId());
	}

	void SamplerObject::deactivate()
	{
		glActiveTexture(GL_TEXTURE0 + _bindingPoint);
		glBindTexture(static_cast<GLenum>(_type), 0);
	}
}
