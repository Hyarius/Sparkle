#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include "rendering/spk_texture.hpp"

namespace spk
{
	class GPUTexture : public spk::Texture
	{
	public:
		static constexpr GLuint InvalidGLId = 0;

	private:
		mutable GLuint _glId = InvalidGLId;

		static void _setupTextureParameters(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap);
		static void _convertFormat(Format p_format, GLint& p_internalFormat, GLenum& p_externalFormat);

		void _uploadToGPU() const;

	protected:
		void _synchronize() const override;

	public:
		GPUTexture();
		~GPUTexture() override;

		GPUTexture(const GPUTexture&) = delete;
		GPUTexture& operator=(const GPUTexture&) = delete;

		GPUTexture(GPUTexture&&) noexcept;
		GPUTexture& operator=(GPUTexture&&) noexcept;

		GLuint glId() const;
	};
}
