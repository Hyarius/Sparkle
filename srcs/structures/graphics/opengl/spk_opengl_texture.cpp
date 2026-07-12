#include "structures/graphics/opengl/spk_opengl_texture.hpp"

namespace spk
{
	namespace OpenGL
	{
		TextureFormat Texture::formatDescriptor(spk::Texture::Format p_format) noexcept
		{
			switch (p_format)
			{
			case spk::Texture::Format::RGB:
				return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, false, false};
			case spk::Texture::Format::RGBA:
				return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false, false};
			case spk::Texture::Format::BGR:
				return {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE, false, false};
			case spk::Texture::Format::BGRA:
				return {GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, false, false};
			case spk::Texture::Format::GreyLevel:
				return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, false, false};
			case spk::Texture::Format::DualChannel:
				return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE, false, false};
			case spk::Texture::Format::Depth24:
				return {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true, false};
			case spk::Texture::Format::Depth32F:
				return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, true, false};
			case spk::Texture::Format::Depth24Stencil8:
				return {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, true, true};
			default:
				return {};
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
				{
					constexpr GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
					glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
				}
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

		Texture::Texture(const spk::Texture &p_source)
		{
			std::shared_ptr<const spk::Texture::Resource> sourceResource = p_source._resourceSnapshot();

			const spk::Vector2UInt &sz = sourceResource->size;
			if (sz.x == 0 || sz.y == 0 || sourceResource->format == spk::Texture::Format::Error)
			{
				return;
			}

			const TextureFormat descriptor = formatDescriptor(sourceResource->format);
			if (descriptor.internalFormat == GL_NONE || descriptor.externalFormat == GL_NONE || descriptor.elementType == GL_NONE)
			{
				return;
			}

			GLint previousBinding = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousBinding);
			glGenTextures(1, &_id);
			try
			{
				glBindTexture(GL_TEXTURE_2D, _id);
				const void *pixels = sourceResource->contentSource == spk::Texture::ContentSource::PixelData &&
											 sourceResource->pixels.empty() == false
										 ? sourceResource->pixels.data()
										 : nullptr;
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					descriptor.internalFormat,
					static_cast<GLsizei>(sz.x),
					static_cast<GLsizei>(sz.y),
					0,
					descriptor.externalFormat,
					descriptor.elementType,
					pixels);
				_setupParameters(sourceResource->filtering, sourceResource->wrap, sourceResource->mipmap);
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousBinding));
			} catch (...)
			{
				glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousBinding));
				glDeleteTextures(1, &_id);
				_id = 0;
				throw;
			}
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
