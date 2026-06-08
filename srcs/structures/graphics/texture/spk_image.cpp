#include "structures/graphics/texture/spk_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <memory>
#include <stdexcept>

namespace spk
{
	Image::Image()
	{
	}

	Image::Image(const std::filesystem::path& p_path)
	{
		loadFromFile(p_path);
	}

	void Image::loadFromFile(const std::filesystem::path& p_path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_uc* rawData = stbi_load(p_path.string().c_str(), &width, &height, &channels, 0);
		if (rawData == nullptr)
		{
			throw std::runtime_error("Image: failed to load file: " + p_path.string());
		}

		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> imageData(rawData, stbi_image_free);

		setPixels(
			imageData.get(),
			spk::Vector2UInt{static_cast<unsigned int>(width), static_cast<unsigned int>(height)},
			_determineFormat(channels),
			Filtering::Nearest,
			Wrap::ClampToEdge,
			Mipmap::Enable);
	}

	void Image::loadFromData(const std::vector<uint8_t>& p_data)
	{
		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_uc* rawData = stbi_load_from_memory(
			reinterpret_cast<const stbi_uc*>(p_data.data()),
			static_cast<int>(p_data.size()),
			&width,
			&height,
			&channels,
			0);

		if (rawData == nullptr)
		{
			throw std::runtime_error("Image: failed to load from raw data.");
		}

		std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> imageData(rawData, stbi_image_free);

		setPixels(
			imageData.get(),
			spk::Vector2UInt{static_cast<unsigned int>(width), static_cast<unsigned int>(height)},
			_determineFormat(channels),
			Filtering::Nearest,
			Wrap::ClampToEdge,
			Mipmap::Enable);
	}

	Texture::Format Image::_determineFormat(int p_channels)
	{
		switch (p_channels)
		{
		case 1:
			return Format::GreyLevel;
		case 2:
			return Format::DualChannel;
		case 3:
			return Format::RGB;
		case 4:
			return Format::RGBA;
		default:
			return Format::Error;
		}
	}
}
