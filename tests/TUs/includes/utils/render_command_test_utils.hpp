#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

#include "sparkle.hpp"
#include "spk_generated_resources.hpp"
#include "test_resource_path_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
#include <GL/glew.h>
#endif

namespace sparkle_test
{
	[[nodiscard]] inline std::filesystem::path renderCommandExpectedPath(const std::string& p_name)
	{
		return spk::test::expectedImagePath("RenderCommands", p_name);
	}

	[[nodiscard]] inline std::filesystem::path renderCommandResultPath(const std::string& p_name)
	{
		return spk::test::resultImagePath("RenderCommands", p_name);
	}

	[[nodiscard]] inline spk::Font testFont()
	{
		const auto& bytes = SPARKLE_GET_RESOURCE("tests/TUs/resources/fonts/arial.ttf");
		return spk::Font::fromRawData(std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
	}

	[[nodiscard]] inline std::size_t countLitPixels(const std::filesystem::path& p_path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char* rawPixels = stbi_load(p_path.string().c_str(), &width, &height, &channels, 4);
		if (rawPixels == nullptr)
		{
			throw std::runtime_error("Failed to load rendered image: " + p_path.string());
		}

		std::size_t result = 0;
		const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
		for (std::size_t i = 0; i < pixelCount; ++i)
		{
			const std::size_t index = i * 4;
			if (rawPixels[index + 0] > 10 || rawPixels[index + 1] > 10 || rawPixels[index + 2] > 10)
			{
				++result;
			}
		}
		stbi_image_free(rawPixels);
		return result;
	}

	[[nodiscard]] inline std::vector<std::uint8_t> makeTwoSpritePngBytes()
	{
		constexpr int width = 32;
		constexpr int height = 16;
		std::vector<std::uint8_t> pixels(
			static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const std::size_t index = (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4;
				const bool rightSprite = x >= width / 2;
				pixels[index + 0] = rightSprite ? 0u : 255u;
				pixels[index + 1] = rightSprite ? 255u : 0u;
				pixels[index + 2] = 0;
				pixels[index + 3] = 255;
			}
		}

		std::vector<std::uint8_t> result;
		stbi_write_png_to_func(
			[](void* p_context, void* p_data, int p_size)
			{
				auto* output = reinterpret_cast<std::vector<std::uint8_t>*>(p_context);
				const auto* bytes = reinterpret_cast<const std::uint8_t*>(p_data);
				output->insert(output->end(), bytes, bytes + p_size);
			},
			&result, width, height, 4, pixels.data(), width * 4);
		return result;
	}

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
	[[nodiscard]] inline std::shared_ptr<spk::OpenGL::Texture> makeSolidTexture(
		const spk::Vector2UInt& p_size,
		std::uint8_t p_red,
		std::uint8_t p_green,
		std::uint8_t p_blue,
		std::uint8_t p_alpha = 255)
	{
		std::vector<std::uint8_t> pixels(
			static_cast<std::size_t>(p_size.x) * static_cast<std::size_t>(p_size.y) * 4, 0);
		for (std::size_t i = 0; i < pixels.size(); i += 4)
		{
			pixels[i + 0] = p_red;
			pixels[i + 1] = p_green;
			pixels[i + 2] = p_blue;
			pixels[i + 3] = p_alpha;
		}

		auto texture = std::make_shared<spk::OpenGL::Texture>();
		texture->setPixels(
			pixels,
			p_size,
			spk::Texture::Format::RGBA,
			spk::Texture::Filtering::Nearest,
			spk::Texture::Wrap::ClampToEdge,
			spk::Texture::Mipmap::Disable);
		return texture;
	}
#endif
}
