#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>
#include <GL/gl.h>

#include "rendering/spk_texture.hpp"

namespace spk::OpenGL
{
	class Texture : public spk::Texture
	{
	private:
		mutable GLuint _glId = 0;

		static void _setupTextureParameters(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap);
		static void _convertFormat(Format p_format, GLint& p_internalFormat, GLenum& p_externalFormat);

		void _uploadToGPU() const;

	protected:
		void _synchronize() const override;

	public:
		Texture();
		~Texture() override;

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&&) noexcept;
		Texture& operator=(Texture&&) noexcept;

		GLuint glId() const;
	};
}

#endif
