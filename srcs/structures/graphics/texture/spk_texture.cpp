#include "structures/graphics/texture/spk_texture.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <stdexcept>
#include <utility>

namespace spk
{
	std::deque<Texture::ID>& Texture::_availableIDs()
	{
		static std::deque<ID> ids;
		return ids;
	}

	Texture::ID& Texture::_nextID()
	{
		static ID next = 0;
		return next;
	}

	Texture::ID Texture::_takeId()
	{
		auto& available = _availableIDs();
		if (available.empty() == false)
		{
			ID result = available.back();
			available.pop_back();
			return result;
		}
		return _nextID()++;
	}

	void Texture::_releaseId(ID p_id)
	{
		if (p_id == InvalidID)
		{
			return;
		}
		_availableIDs().push_back(p_id);
	}

	size_t Texture::_getBytesPerPixel(Format p_format) const
	{
		switch (p_format)
		{
		case Format::GreyLevel:
			return 1;
		case Format::DualChannel:
			return 2;
		case Format::RGB:
		case Format::BGR:
			return 3;
		case Format::RGBA:
		case Format::BGRA:
			return 4;
		default:
			return 0;
		}
	}

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
			glTexParameteri(
				GL_TEXTURE_2D,
				GL_TEXTURE_MIN_FILTER,
				p_mipmap == Mipmap::Enable ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case Filtering::Linear:
			glTexParameteri(
				GL_TEXTURE_2D,
				GL_TEXTURE_MIN_FILTER,
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

		if (_glId == InvalidGLId)
		{
			glGenTextures(1, &_glId);
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

	const Texture::Section Texture::Section::whole = Texture::Section({0.0f, 0.0f}, {1.0f, 1.0f});

	Texture::Section::Section() :
		anchor(0.0f, 0.0f),
		size(0.0f, 0.0f)
	{
	}

	Texture::Section::Section(spk::Vector2 p_anchor, spk::Vector2 p_size) :
		anchor(p_anchor),
		size(p_size)
	{
	}

	bool Texture::Section::operator==(const Section& p_other) const noexcept
	{
		return anchor == p_other.anchor && size == p_other.size;
	}

	bool Texture::Section::operator!=(const Section& p_other) const noexcept
	{
		return (*this == p_other) == false;
	}

	Texture::Texture() :
		_id(_takeId()),
		_size{0, 0},
		_format(Format::Error),
		_filtering(Filtering::Nearest),
		_wrap(Wrap::ClampToEdge),
		_mipmap(Mipmap::Enable)
	{
	}

	Texture::~Texture()
	{
		if (_glId != InvalidGLId)
		{
			glDeleteTextures(1, &_glId);
			_glId = InvalidGLId;
		}
		_releaseId(_id);
	}

	Texture::Texture(Texture&& p_other) noexcept :
		SynchronizableTrait(std::move(p_other)),
		_id(p_other._id),
		_pixels(std::move(p_other._pixels)),
		_size(p_other._size),
		_format(p_other._format),
		_filtering(p_other._filtering),
		_wrap(p_other._wrap),
		_mipmap(p_other._mipmap),
		_glId(std::exchange(p_other._glId, InvalidGLId))
	{
		p_other._id = InvalidID;
	}

	Texture& Texture::operator=(Texture&& p_other) noexcept
	{
		if (this != &p_other)
		{
			if (_glId != InvalidGLId)
			{
				glDeleteTextures(1, &_glId);
			}
			_releaseId(_id);

			SynchronizableTrait::operator=(std::move(p_other));
			_id = p_other._id;
			_pixels = std::move(p_other._pixels);
			_size = p_other._size;
			_format = p_other._format;
			_filtering = p_other._filtering;
			_wrap = p_other._wrap;
			_mipmap = p_other._mipmap;
			_glId = std::exchange(p_other._glId, InvalidGLId);

			p_other._id = InvalidID;
		}
		return *this;
	}

	void Texture::setPixels(
		const std::vector<uint8_t>& p_data,
		const spk::Vector2UInt& p_size,
		Format p_format,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap)
	{
		_pixels = p_data;
		_size = p_size;
		_format = p_format;
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
		requestSynchronization();
	}

	void Texture::setPixels(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, Format p_format)
	{
		_pixels = p_data;
		_size = p_size;
		_format = p_format;
		requestSynchronization();
	}

	void Texture::setPixels(
		const uint8_t* p_data,
		const spk::Vector2UInt& p_size,
		Format p_format,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap)
	{
		_size = p_size;
		_format = p_format;
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;

		const size_t byteCount = p_size.x * p_size.y * _getBytesPerPixel(p_format);
		if (p_data != nullptr)
		{
			_pixels.assign(p_data, p_data + byteCount);
		}
		else
		{
			_pixels.assign(byteCount, 0);
		}
		requestSynchronization();
	}

	void Texture::setPixels(const uint8_t* p_data, const spk::Vector2UInt& p_size, Format p_format)
	{
		_size = p_size;
		_format = p_format;

		const size_t byteCount = p_size.x * p_size.y * _getBytesPerPixel(p_format);
		if (p_data != nullptr)
		{
			_pixels.assign(p_data, p_data + byteCount);
		}
		else
		{
			_pixels.assign(byteCount, 0);
		}
		requestSynchronization();
	}

	void Texture::setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap)
	{
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
		requestSynchronization();
	}

	Texture::ID Texture::id() const
	{
		return _id;
	}

	const std::vector<uint8_t>& Texture::pixels() const
	{
		return _pixels;
	}

	const spk::Vector2UInt& Texture::size() const
	{
		return _size;
	}

	Texture::Format Texture::format() const
	{
		return _format;
	}

	Texture::Filtering Texture::filtering() const
	{
		return _filtering;
	}

	Texture::Wrap Texture::wrap() const
	{
		return _wrap;
	}

	Texture::Mipmap Texture::mipmap() const
	{
		return _mipmap;
	}

	GLuint Texture::glId() const
	{
		return _glId;
	}

	void Texture::saveAsPng(const std::filesystem::path& p_path) const
	{
		if (_pixels.empty() || _size.x == 0 || _size.y == 0)
		{
			throw std::runtime_error("Cannot save texture: no pixel data.");
		}

		const int channels = static_cast<int>(_getBytesPerPixel(_format));
		if (channels == 0)
		{
			throw std::runtime_error("Unsupported texture format for PNG export.");
		}

		std::filesystem::create_directories(p_path.parent_path());
		const int stride = static_cast<int>(_size.x) * channels;
		if (stbi_write_png(p_path.string().c_str(), static_cast<int>(_size.x), static_cast<int>(_size.y), channels, _pixels.data(), stride) == 0)
		{
			throw std::runtime_error("Failed to write PNG file at: " + p_path.string());
		}
	}
}
