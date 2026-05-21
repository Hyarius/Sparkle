#include "image_comparison_test_utils.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

namespace
{
	struct LoadedImage
	{
		int width = 0;
		int height = 0;
		std::vector<std::uint8_t> pixels;
	};

	[[nodiscard]] LoadedImage loadImage(const std::filesystem::path& p_path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char* rawPixels = stbi_load(p_path.string().c_str(), &width, &height, &channels, 4);
		if (rawPixels == nullptr)
		{
			throw std::runtime_error("Failed to load image [" + p_path.string() + "]: " + stbi_failure_reason());
		}

		LoadedImage result;
		result.width = width;
		result.height = height;
		result.pixels.assign(rawPixels, rawPixels + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4));
		stbi_image_free(rawPixels);
		return result;
	}

	[[nodiscard]] bool channelDiffers(std::uint8_t p_left, std::uint8_t p_right, std::uint8_t p_tolerance)
	{
		return std::abs(static_cast<int>(p_left) - static_cast<int>(p_right)) > static_cast<int>(p_tolerance);
	}

	[[nodiscard]] bool pixelDiffers(
		const std::uint8_t* p_actual,
		const std::uint8_t* p_expected,
		sparkle_test::ImageComparisonOptions p_options)
	{
		if (channelDiffers(p_actual[3], p_expected[3], p_options.alphaTolerance) == true)
		{
			return true;
		}

		if (p_actual[3] <= p_options.transparentAlphaThreshold && p_expected[3] <= p_options.transparentAlphaThreshold)
		{
			return false;
		}

		for (std::size_t channel = 0; channel < 3; ++channel)
		{
			if (channelDiffers(p_actual[channel], p_expected[channel], p_options.rgbTolerance) == true)
			{
				return true;
			}
		}

		return false;
	}
}

namespace sparkle_test
{
	ImageComparisonResult compareImages(
		const std::filesystem::path& p_actualPath,
		const std::filesystem::path& p_expectedPath,
		const std::filesystem::path& p_differencePath,
		ImageComparisonOptions p_options)
	{
		const LoadedImage actual = loadImage(p_actualPath);
		const LoadedImage expected = loadImage(p_expectedPath);

		ImageComparisonResult result;
		result.actualWidth = actual.width;
		result.actualHeight = actual.height;
		result.expectedWidth = expected.width;
		result.expectedHeight = expected.height;

		const int diffWidth = std::max(actual.width, expected.width);
		const int diffHeight = std::max(actual.height, expected.height);
		std::vector<std::uint8_t> difference(static_cast<std::size_t>(diffWidth) * static_cast<std::size_t>(diffHeight) * 4, 0);
		for (std::size_t index = 3; index < difference.size(); index += 4)
		{
			difference[index] = 255;
		}

		for (int y = 0; y < diffHeight; ++y)
		{
			for (int x = 0; x < diffWidth; ++x)
			{
				bool pixelMatches = x < actual.width &&
									y < actual.height &&
									x < expected.width &&
									y < expected.height;

				if (pixelMatches == true)
				{
					const std::size_t actualIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(actual.width) + static_cast<std::size_t>(x)) * 4;
					const std::size_t expectedIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(expected.width) + static_cast<std::size_t>(x)) * 4;
					pixelMatches = (pixelDiffers(actual.pixels.data() + actualIndex, expected.pixels.data() + expectedIndex, p_options) == false);
				}

				if (pixelMatches == false)
				{
					const std::size_t diffIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(diffWidth) + static_cast<std::size_t>(x)) * 4;
					difference[diffIndex + 0] = 255;
					difference[diffIndex + 1] = 0;
					difference[diffIndex + 2] = 0;
					difference[diffIndex + 3] = 255;
					++result.differentPixelCount;
				}
			}
		}

		result.matches = result.differentPixelCount == 0;

		if (stbi_write_png(p_differencePath.string().c_str(), diffWidth, diffHeight, 4, difference.data(), diffWidth * 4) == 0)
		{
			throw std::runtime_error("Failed to write image comparison diff [" + p_differencePath.string() + "]");
		}

		return result;
	}
}
