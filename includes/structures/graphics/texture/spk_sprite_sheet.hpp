#pragma once

#include <filesystem>
#include <stdexcept>
#include <vector>

#include "structures/graphics/texture/spk_image.hpp"

namespace spk
{
	class SpriteSheet : public Image
	{
	public:
		using Sprite = Texture::Section;

	private:
		spk::Vector2UInt _nbSprite;
		spk::Vector2 _unit;
		std::vector<Sprite> _sprites;

		using Image::loadFromData;
		using Image::loadFromFile;

		void _buildSprites(const spk::Vector2UInt& p_spriteCount)
		{
			if (p_spriteCount.x == 0 || p_spriteCount.y == 0)
			{
				throw std::invalid_argument("SpriteSheet: sprite count cannot be zero.");
			}

			_nbSprite = p_spriteCount;
			_unit = spk::Vector2(1.0f, 1.0f) / spk::Vector2(static_cast<float>(_nbSprite.x), static_cast<float>(_nbSprite.y));

			const spk::Vector2 halfPixel = 0.5f / spk::Vector2(static_cast<float>(size().x), static_cast<float>(size().y));

			_sprites.clear();

			if (p_spriteCount == spk::Vector2UInt(1, 1))
			{
				_sprites.push_back({spk::Vector2(0.0f, 0.0f) + halfPixel, _unit - halfPixel});
			}
			else
			{
				for (size_t y = 0; y < _nbSprite.y; ++y)
				{
					for (size_t x = 0; x < _nbSprite.x; ++x)
					{
						const spk::Vector2 anchor = spk::Vector2(static_cast<float>(x), static_cast<float>(y)) * _unit;
						_sprites.push_back({anchor + halfPixel, _unit - halfPixel});
					}
				}
			}
		}

	public:
		static SpriteSheet fromRawData(
			const std::vector<uint8_t>& p_rawData,
			const spk::Vector2UInt& p_spriteCount,
			Filtering p_filtering = Filtering::Nearest,
			Wrap p_wrap = Wrap::ClampToEdge,
			Mipmap p_mipmap = Mipmap::Enable)
		{
			SpriteSheet result;
			result.loadFromData(p_rawData, p_spriteCount);
			result.setProperties(p_filtering, p_wrap, p_mipmap);
			return result;
		}

		SpriteSheet() = default;

		SpriteSheet(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteCount)
		{
			loadFromFile(p_path, p_spriteCount);
		}

		void loadFromFile(const std::filesystem::path& p_path, const spk::Vector2UInt& p_spriteCount)
		{
			Image::loadFromFile(p_path);
			_buildSprites(p_spriteCount);
		}

		void loadFromData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_spriteCount)
		{
			Image::loadFromData(p_data);
			_buildSprites(p_spriteCount);
		}

		const spk::Vector2UInt& nbSprite() const
		{
			return _nbSprite;
		}

		const spk::Vector2& unit() const
		{
			return _unit;
		}

		const std::vector<Sprite>& sprites() const
		{
			return _sprites;
		}

		size_t spriteID(const spk::Vector2UInt& p_coord) const
		{
			if (p_coord.x >= _nbSprite.x || p_coord.y >= _nbSprite.y)
			{
				throw std::out_of_range("SpriteSheet: sprite coordinates out of range.");
			}
			return _nbSprite.x * p_coord.y + p_coord.x;
		}

		const Sprite& sprite(const spk::Vector2UInt& p_coord) const
		{
			return sprite(spriteID(p_coord));
		}

		const Sprite& sprite(size_t p_spriteID) const
		{
			if (p_spriteID >= _sprites.size())
			{
				throw std::out_of_range("SpriteSheet: sprite ID out of range.");
			}
			return _sprites[p_spriteID];
		}
	};
}
