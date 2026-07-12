#pragma once

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	namespace OpenGL
	{
		struct TextureFormat
		{
			GLint internalFormat = GL_NONE;
			GLenum externalFormat = GL_NONE;
			GLenum elementType = GL_NONE;
			bool depth = false;
			bool stencil = false;
		};

		class Texture : public Object
		{
		private:
			GLuint _id = 0;

			static void _setupParameters(spk::Texture::Filtering p_filtering, spk::Texture::Wrap p_wrap, spk::Texture::Mipmap p_mipmap);

		public:
			[[nodiscard]] static TextureFormat formatDescriptor(spk::Texture::Format p_format) noexcept;

			explicit Texture(const spk::Texture &p_source);
			~Texture() override;

			Texture(const Texture &) = delete;
			Texture &operator=(const Texture &) = delete;
			Texture(Texture &&) = delete;
			Texture &operator=(Texture &&) = delete;

			[[nodiscard]] GLuint id() const noexcept;
		};
	}
}
