#include "structures/graphics/opengl/spk_opengl_texture.hpp"

namespace spk
{
	namespace OpenGL
	{
		size_t Texture::_bytesPerPixel(spk::Texture::Format p_format)
		{
			switch (p_format)
			{
			case spk::Texture::Format::GreyLevel:
				return 1;
			case spk::Texture::Format::DualChannel:
				return 2;
			case spk::Texture::Format::RGB:
			case spk::Texture::Format::BGR:
				return 3;
			case spk::Texture::Format::RGBA:
			case spk::Texture::Format::BGRA:
				return 4;
			default:
				return 0;
			}
		}

		void Texture::_convertFormat(spk::Texture::Format p_format, GLint& p_internalFormat, GLenum& p_externalFormat)
		{
			switch (p_format)
			{
			case spk::Texture::Format::RGB:
				p_internalFormat = GL_RGB;
				p_externalFormat = GL_RGB;
				break;
			case spk::Texture::Format::RGBA:
				p_internalFormat = GL_RGBA;
				p_externalFormat = GL_RGBA;
				break;
			case spk::Texture::Format::BGR:
				p_internalFormat = GL_RGB;
				p_externalFormat = GL_BGR;
				break;
			case spk::Texture::Format::BGRA:
				p_internalFormat = GL_RGBA;
				p_externalFormat = GL_BGRA;
				break;
			case spk::Texture::Format::GreyLevel:
				p_internalFormat = GL_RED;
				p_externalFormat = GL_RED;
				break;
			case spk::Texture::Format::DualChannel:
				p_internalFormat = GL_RG;
				p_externalFormat = GL_RG;
				break;
			default:
				p_internalFormat = GL_NONE;
				p_externalFormat = GL_NONE;
				break;
			}
		}

		void Texture::_setupParameters(spk::Texture::Filtering p_filtering, spk::Texture::Wrap p_wrap, spk::Texture::Mipmap p_mipmap)
		{
			switch (p_wrap)
			{
			case spk::Texture::Wrap::Repeat:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			case spk::Texture::Wrap::ClampToEdge:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case spk::Texture::Wrap::ClampToBorder:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				break;
			}

			switch (p_filtering)
			{
			case spk::Texture::Filtering::Nearest:
				glTexParameteri(
					GL_TEXTURE_2D,
					GL_TEXTURE_MIN_FILTER,
					p_mipmap == spk::Texture::Mipmap::Enable ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			case spk::Texture::Filtering::Linear:
				glTexParameteri(
					GL_TEXTURE_2D,
					GL_TEXTURE_MIN_FILTER,
					p_mipmap == spk::Texture::Mipmap::Enable ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			}

			if (p_mipmap == spk::Texture::Mipmap::Enable)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}
		}

		Texture::Texture(const spk::Texture& p_source)
		{
			std::shared_ptr<const spk::Texture::Resource> sourceResource = p_source._resourceSnapshot();

			const spk::Vector2UInt& sz = sourceResource->size;
			if (sz.x == 0 || sz.y == 0 || sourceResource->format == spk::Texture::Format::Error)
			{
				return;
			}

			GLint internalFormat = GL_NONE;
			GLenum externalFormat = GL_NONE;
			_convertFormat(sourceResource->format, internalFormat, externalFormat);
			if (internalFormat == GL_NONE || externalFormat == GL_NONE)
			{
				return;
			}

			glGenTextures(1, &_id);
			glBindTexture(GL_TEXTURE_2D, _id);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				internalFormat,
				static_cast<GLsizei>(sz.x),
				static_cast<GLsizei>(sz.y),
				0,
				externalFormat,
				GL_UNSIGNED_BYTE,
				sourceResource->pixels.data());
			_setupParameters(sourceResource->filtering, sourceResource->wrap, sourceResource->mipmap);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		Texture::~Texture()
		{
			if (_id != 0 && _ownsCurrentContext() == true)
			{
				glDeleteTextures(1, &_id);
			}
		}

		GLuint Texture::id() const noexcept
		{
			return _id;
		}
	}
}
