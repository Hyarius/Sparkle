#include "structures/graphics/spk_sampler_object.hpp"

#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	SamplerObject::SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint) :
		_designator(p_name),
		_bindingPoint(p_bindingPoint),
		_type(p_type)
	{
	}

	void SamplerObject::bind(const spk::Texture& p_texture)
	{
		_texture = &p_texture;
	}

	const spk::Texture* SamplerObject::texture() const noexcept
	{
		return _texture;
	}

	const std::string& SamplerObject::designator() const noexcept
	{
		return _designator;
	}

	SamplerObject::BindingPoint SamplerObject::bindingPoint() const
	{
		return _bindingPoint;
	}

	void SamplerObject::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
		_gpu.invalidateUniformLocation();
	}

	SamplerObject::Type SamplerObject::type() const
	{
		return _type;
	}

	void SamplerObject::setType(Type p_type)
	{
		_type = p_type;
	}

	void SamplerObject::activate(const spk::RenderContext& p_context)
	{
		_gpu.activate(p_context, *this);
	}

	void SamplerObject::deactivate()
	{
		_gpu.deactivate(*this);
	}
}
