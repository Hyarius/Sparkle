#pragma once

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	namespace OpenGL
	{
		// GPU-side texture. Bound to the RenderContext that uploaded it and only
		// usable while that context is current.
		class Texture : public Object
		{
		private:
			GLuint _id = 0;

			static size_t _bytesPerPixel(spk::Texture::Format p_format);
			static void _convertFormat(spk::Texture::Format p_format, GLint &p_internalFormat, GLenum &p_externalFormat);
			static void _setupParameters(spk::Texture::Filtering p_filtering, spk::Texture::Wrap p_wrap, spk::Texture::Mipmap p_mipmap);

		public:
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
