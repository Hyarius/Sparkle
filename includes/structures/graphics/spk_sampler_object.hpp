#pragma once

#include <string>

#include "structures/graphics/opengl/spk_opengl_sampler_object.hpp"

namespace spk
{
	class RenderContext;
	class Program;
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
		BindingPoint _bindingPoint;
		Type _type;
		const spk::Program& _program;
		mutable spk::OpenGL::SamplerObject _gpu;

	public:
		SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint, const spk::Program& p_program);

		void bind(const spk::Texture& p_texture);

		[[nodiscard]] const spk::Texture* texture() const noexcept;
		[[nodiscard]] const std::string& designator() const noexcept;
		[[nodiscard]] const spk::Program& program() const noexcept;
		[[nodiscard]] BindingPoint bindingPoint() const noexcept;
		[[nodiscard]] Type type() const noexcept;

		void activate(const spk::RenderContext& p_context) const;
		void deactivate() const;
	};
}
