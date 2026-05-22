#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <string>

#include <GL/glew.h>
#include <GL/gl.h>

#include "opengl/spk_opengl_texture.hpp"

namespace spk::OpenGL
{
	class SamplerObject
	{
	public:
		enum class Type : GLenum
		{
			Texture1D = GL_TEXTURE_1D,
			Texture2D = GL_TEXTURE_2D,
			Texture3D = GL_TEXTURE_3D,
			TextureCubeMap = GL_TEXTURE_CUBE_MAP
		};

		using BindingPoint = int;

	private:
		const OpenGL::Texture* _texture = nullptr;
		std::string _designator;
		BindingPoint _bindingPoint = -1;
		GLint _uniformDestination = -1;
		Type _type = Type::Texture2D;

	public:
		SamplerObject() = default;
		SamplerObject(const std::string& p_name, Type p_type, BindingPoint p_bindingPoint);

		void bind(const OpenGL::Texture& p_texture);

		BindingPoint bindingPoint() const;
		void setBindingPoint(BindingPoint p_bindingPoint);

		Type type() const;
		void setType(Type p_type);

		void activate();
		void deactivate();
	};
}

#endif
