#pragma once

#include <string>

#include "structures/graphics/opengl/spk_opengl_sampler_object.hpp"

namespace spk
{
	class RenderContext;
	class Texture;

	class SamplerObject
	{
	public:
		enum class Type
		{
			Texture1D,
			Texture2D,
			Texture3D,
			TextureCubeMap
		};

		using BindingPoint = int;

	private:
		const spk::Texture* _texture = nullptr;
		std::string _designator;
		BindingPoint _bindingPoint = -1;
		Type _type = Type::Texture2D;
		mutable spk::OpenGL::SamplerObject _gpu;

	public:
		SamplerObject() = default;
		SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint);

		void bind(const spk::Texture& p_texture);

		[[nodiscard]] const spk::Texture* texture() const noexcept;
		[[nodiscard]] const std::string& designator() const noexcept;

		BindingPoint bindingPoint() const;
		void setBindingPoint(BindingPoint p_bindingPoint);

		Type type() const;
		void setType(Type p_type);

		void activate(const spk::RenderContext& p_context);
		void deactivate();
	};
}
