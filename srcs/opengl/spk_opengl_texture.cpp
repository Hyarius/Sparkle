#include "opengl/spk_opengl_texture.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <utility>

namespace spk::OpenGL
{
	void Texture::_convertFormat(Format p_format, GLint& p_internalFormat, GLenum& p_externalFormat)
	{
		switch (p_format)
		{
		case Format::RGB:
			p_internalFormat = GL_RGB;
			p_externalFormat = GL_RGB;
			break;
		case Format::RGBA:
			p_internalFormat = GL_RGBA;
			p_externalFormat = GL_RGBA;
			break;
		case Format::BGR:
			p_internalFormat = GL_RGB;
			p_externalFormat = GL_BGR;
			break;
		case Format::BGRA:
			p_internalFormat = GL_RGBA;
			p_externalFormat = GL_BGRA;
			break;
		case Format::GreyLevel:
			p_internalFormat = GL_RED;
			p_externalFormat = GL_RED;
			break;
		case Format::DualChannel:
			p_internalFormat = GL_RG;
			p_externalFormat = GL_RG;
			break;
		default:
			p_internalFormat = GL_NONE;
			p_externalFormat = GL_NONE;
			break;
		}
	}

	void Texture::_setupTextureParameters(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap)
	{
		switch (p_wrap)
		{
		case Wrap::Repeat:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		case Wrap::ClampToEdge:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			break;
		case Wrap::ClampToBorder:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			break;
		}

		switch (p_filtering)
		{
		case Filtering::Nearest:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				p_mipmap == Mipmap::Enable ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case Filtering::Linear:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				p_mipmap == Mipmap::Enable ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}

		if (p_mipmap == Mipmap::Enable)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	void Texture::_uploadToGPU() const
	{
		if (size().x == 0 || size().y == 0 || format() == Format::Error)
		{
			return;
		}

		GLint internalFormat = GL_NONE;
		GLenum externalFormat = GL_NONE;
		_convertFormat(format(), internalFormat, externalFormat);

		if (internalFormat == GL_NONE || externalFormat == GL_NONE)
		{
			return;
		}

		glBindTexture(GL_TEXTURE_2D, _glId);

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			internalFormat,
			static_cast<GLsizei>(size().x),
			static_cast<GLsizei>(size().y),
			0,
			externalFormat,
			GL_UNSIGNED_BYTE,
			pixels().data());

		_setupTextureParameters(filtering(), wrap(), mipmap());

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::_synchronize() const
	{
		_uploadToGPU();
	}

	Texture::Texture() :
		spk::Texture()
	{
		glGenTextures(1, &_glId);
	}

	Texture::~Texture()
	{
		if (_glId != 0)
		{
			glDeleteTextures(1, &_glId);
			_glId = 0;
		}
	}

	Texture::Texture(Texture&& p_other) noexcept :
		spk::Texture(std::move(p_other)),
		_glId(std::exchange(p_other._glId, 0))
	{
	}

	Texture& Texture::operator=(Texture&& p_other) noexcept
	{
		if (this != &p_other)
		{
			if (_glId != 0)
			{
				glDeleteTextures(1, &_glId);
			}
			spk::Texture::operator=(std::move(p_other));
			_glId = std::exchange(p_other._glId, 0);
		}
		return *this;
	}

	GLuint Texture::glId() const
	{
		return _glId;
	}
}

#endif
