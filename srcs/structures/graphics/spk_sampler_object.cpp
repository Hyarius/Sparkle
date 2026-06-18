#include "structures/graphics/spk_sampler_object.hpp"

#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	SamplerObject::SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint, const spk::Program& p_program) :
		_designator(p_name),
		_bindingPoint(p_bindingPoint),
		_type(p_type),
		_program(p_program)
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

	const spk::Program& SamplerObject::program() const noexcept
	{
		return _program;
	}

	SamplerObject::BindingPoint SamplerObject::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	SamplerObject::Type SamplerObject::type() const noexcept
	{
		return _type;
	}

	void SamplerObject::activate(const spk::RenderContext& p_context) const
	{
		_gpu.activate(p_context, *this);
	}

	void SamplerObject::deactivate() const
	{
		_gpu.deactivate(*this);
	}
}
