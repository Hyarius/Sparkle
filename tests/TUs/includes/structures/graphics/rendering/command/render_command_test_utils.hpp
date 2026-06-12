#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <stb_image_write.h>

#include "sparkle.hpp"
#include "spk_generated_resources.hpp"
#include "utils/test_resource_path_utils.hpp"

#include <GL/glew.h>

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

	// Enough distinct codepoints to overflow a freshly created atlas and force it
	// to grow (which rescales every glyph UV it stores).
	[[nodiscard]] inline spk::Font::Text manyGlyphsText()
	{
		spk::Font::Text text;
		for (char32_t codepoint = U'!'; codepoint <= U'~'; ++codepoint)
		{
			text.push_back(codepoint);
		}
		for (char32_t codepoint = 0x00C0; codepoint <= 0x00FF; ++codepoint) // Latin-1 letters
		{
			text.push_back(codepoint);
		}
		return text;
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

	[[nodiscard]] inline std::shared_ptr<spk::Texture> makeSolidTexture(
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

		auto texture = std::make_shared<spk::Texture>();
		texture->setPixels(
			pixels,
			p_size,
			spk::Texture::Format::RGBA,
			spk::Texture::Filtering::Nearest,
			spk::Texture::Wrap::ClampToEdge,
			spk::Texture::Mipmap::Disable);
		return texture;
	}
}
