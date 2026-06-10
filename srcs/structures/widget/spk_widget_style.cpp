#include "structures/widget/spk_widget_style.hpp"

#include <stdexcept>
#include <string>
#include <utility>

#include "spk_generated_resources.hpp"

namespace
{
	[[nodiscard]] std::string keyFrom(std::string_view p_name)
	{
		return std::string(p_name);
	}

	[[nodiscard]] spk::Vector2Int spriteCornerSize(const spk::SpriteSheet& p_spriteSheet)
	{
		return {
			static_cast<int>(p_spriteSheet.size().x / p_spriteSheet.nbSprite().x),
			static_cast<int>(p_spriteSheet.size().y / p_spriteSheet.nbSprite().y)
		};
	}

	void validateSpriteSheet(const std::shared_ptr<spk::SpriteSheet>& p_spriteSheet, const char* p_name)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument(std::string(p_name) + " cannot be null");
		}

		if (p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 3})
		{
			throw std::invalid_argument(std::string(p_name) + " must contain a 3x3 grid");
		}
	}

	void validateCornerSize(const spk::Vector2Int& p_cornerSize, const char* p_name)
	{
		if (p_cornerSize.x < 0 || p_cornerSize.y < 0)
		{
			throw std::invalid_argument(std::string(p_name) + " cannot be negative");
		}
	}

	[[nodiscard]] bool sameColor(const spk::Color& p_left, const spk::Color& p_right)
	{
		return p_left.r == p_right.r &&
			p_left.g == p_right.g &&
			p_left.b == p_right.b &&
			p_left.a == p_right.a;
	}
}

namespace spk
{
	WidgetStyle::WidgetStyle() :
		_textSize(16, 0),
		_glyphColor(1.0f, 1.0f, 1.0f, 1.0f),
		_outlineColor(0.0f, 0.0f, 0.0f, 0.0f),
		_textPadding(0, 0)
	{
	}

	WidgetStyle::WidgetStyle(const WidgetStyle& p_other)
	{
		_copyValuesFrom(p_other);
	}

	WidgetStyle& WidgetStyle::operator=(const WidgetStyle& p_other)
	{
		if (this != &p_other)
		{
			_copyValuesFrom(p_other);
		}

		return *this;
	}

	WidgetStyle::WidgetStyle(WidgetStyle&& p_other) noexcept
	{
		*this = std::move(p_other);
	}

	WidgetStyle& WidgetStyle::operator=(WidgetStyle&& p_other) noexcept
	{
		if (this != &p_other)
		{
			_nineSliceSpriteSheet = std::move(p_other._nineSliceSpriteSheet);
			_nineSliceCornerSize = p_other._nineSliceCornerSize;
			_iconSpriteSheet = std::move(p_other._iconSpriteSheet);
			_font = std::move(p_other._font);
			_textSize = p_other._textSize;
			_glyphColor = p_other._glyphColor;
			_outlineColor = p_other._outlineColor;
			_textPadding = p_other._textPadding;
		}

		return *this;
	}

	void WidgetStyle::_copyValuesFrom(const spk::WidgetStyle& p_other)
	{
		_nineSliceSpriteSheet = p_other._nineSliceSpriteSheet;
		_nineSliceCornerSize = p_other._nineSliceCornerSize;
		_iconSpriteSheet = p_other._iconSpriteSheet;
		_font = p_other._font;
		_textSize = p_other._textSize;
		_glyphColor = p_other._glyphColor;
		_outlineColor = p_other._outlineColor;
		_textPadding = p_other._textPadding;
	}

	WidgetStyle WidgetStyle::makeDefault()
	{
		WidgetStyle result;
		result._nineSliceSpriteSheet = std::make_shared<spk::SpriteSheet>(
			spk::SpriteSheet::fromRawData(
				SPARKLE_GET_RESOURCE("resources/textures/default_nine_slice.png"),
				spk::Vector2UInt{3, 3}));
		result._nineSliceCornerSize = spriteCornerSize(*result._nineSliceSpriteSheet);
		result._iconSpriteSheet = std::make_shared<spk::SpriteSheet>(
			spk::SpriteSheet::fromRawData(
				SPARKLE_GET_RESOURCE("resources/textures/default_iconset.png"),
				spk::Vector2UInt{10, 10},
				spk::SpriteSheet::Filtering::Linear));
		result._font = std::make_shared<spk::Font>(
			spk::Font::fromRawData(SPARKLE_GET_RESOURCE("resources/fonts/arial.ttf")));

		return result;
	}

	namespace
	{
		[[nodiscard]] spk::WidgetStyle makeNineSliceVariant(const char* p_texturePath)
		{
			spk::WidgetStyle result = spk::WidgetStyle::makeDefault();
			auto spriteSheet = std::make_shared<spk::SpriteSheet>(
				spk::SpriteSheet::fromRawData(
					SPARKLE_GET_RESOURCE(p_texturePath),
					spk::Vector2UInt{3, 3}));
			result.setNineSliceSpriteSheet(std::move(spriteSheet));
			return result;
		}
	}

	WidgetStyle WidgetStyle::makeDefaultPressed()
	{
		return makeNineSliceVariant("resources/textures/default_nine_slice_darker.png");
	}

	WidgetStyle WidgetStyle::makeDefaultLight()
	{
		return makeNineSliceVariant("resources/textures/default_nine_slice_light.png");
	}

	WidgetStyle WidgetStyle::makeDefaultDark()
	{
		return makeNineSliceVariant("resources/textures/default_nine_slice_dark.png");
	}

	WidgetStyle WidgetStyle::makeDefaultSliderBody()
	{
		return makeNineSliceVariant("resources/textures/default_slider_body.png");
	}

	void WidgetStyle::notifyEdition() const
	{
		_editionProvider.trigger(this);
	}

	WidgetStyle::Contract WidgetStyle::subscribeToEdition(Callback p_callback) const
	{
		return _editionProvider.subscribe(
			[p_callback = std::move(p_callback)](const spk::WidgetStyle* p_style)
			{
				if (p_style != nullptr)
				{
					p_callback(*p_style);
				}
			});
	}

	void WidgetStyle::setNineSliceSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		validateSpriteSheet(p_spriteSheet, "WidgetStyle nine-slice sprite sheet");
		_nineSliceSpriteSheet = std::move(p_spriteSheet);
		_nineSliceCornerSize = spriteCornerSize(*_nineSliceSpriteSheet);
		notifyEdition();
	}

	void WidgetStyle::setNineSliceCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		validateCornerSize(p_cornerSize, "WidgetStyle nine-slice corner size");
		if (_nineSliceCornerSize == p_cornerSize)
		{
			return;
		}
		_nineSliceCornerSize = p_cornerSize;
		notifyEdition();
	}

	void WidgetStyle::setIconSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("WidgetStyle icon sprite sheet cannot be null");
		}
		_iconSpriteSheet = std::move(p_spriteSheet);
		notifyEdition();
	}

	void WidgetStyle::setFont(std::shared_ptr<spk::Font> p_font)
	{
		if (p_font == nullptr)
		{
			throw std::invalid_argument("WidgetStyle font cannot be null");
		}
		_font = std::move(p_font);
		notifyEdition();
	}

	void WidgetStyle::setTextSize(const spk::Font::Size& p_textSize)
	{
		if (_textSize == p_textSize)
		{
			return;
		}
		_textSize = p_textSize;
		notifyEdition();
	}

	void WidgetStyle::setGlyphColor(const spk::Color& p_color)
	{
		if (sameColor(_glyphColor, p_color) == true)
		{
			return;
		}
		_glyphColor = p_color;
		notifyEdition();
	}

	void WidgetStyle::setOutlineColor(const spk::Color& p_color)
	{
		if (sameColor(_outlineColor, p_color) == true)
		{
			return;
		}
		_outlineColor = p_color;
		notifyEdition();
	}

	void WidgetStyle::setTextPadding(const spk::Vector2Int& p_padding)
	{
		if (_textPadding == p_padding)
		{
			return;
		}
		_textPadding = p_padding;
		notifyEdition();
	}

	const std::shared_ptr<spk::SpriteSheet>& WidgetStyle::nineSliceSpriteSheet() const { return _nineSliceSpriteSheet; }
	const spk::Vector2Int& WidgetStyle::nineSliceCornerSize() const { return _nineSliceCornerSize; }
	const std::shared_ptr<spk::SpriteSheet>& WidgetStyle::iconSpriteSheet() const { return _iconSpriteSheet; }
	const std::shared_ptr<spk::Font>& WidgetStyle::font() const { return _font; }
	const spk::Font::Size& WidgetStyle::textSize() const { return _textSize; }
	const spk::Color& WidgetStyle::glyphColor() const { return _glyphColor; }
	const spk::Color& WidgetStyle::outlineColor() const { return _outlineColor; }
	const spk::Vector2Int& WidgetStyle::textPadding() const { return _textPadding; }

	WidgetStyle::Collection::Collection()
	{
		_styles.emplace(std::string(Default), spk::WidgetStyle::makeDefault());
		_styles.emplace(std::string(DefaultPressed), spk::WidgetStyle::makeDefaultPressed());
		_styles.emplace(std::string(DefaultLight), spk::WidgetStyle::makeDefaultLight());
		_styles.emplace(std::string(DefaultDark), spk::WidgetStyle::makeDefaultDark());
		_styles.emplace(std::string(DefaultSliderBody), spk::WidgetStyle::makeDefaultSliderBody());
	}

	WidgetStyle::Collection& WidgetStyle::Collection::instance()
	{
		static WidgetStyle::Collection result;
		return result;
	}

	bool WidgetStyle::Collection::contains(std::string_view p_name)
	{
		return instance()._styles.contains(keyFrom(p_name));
	}

	spk::WidgetStyle& WidgetStyle::Collection::style(std::string_view p_name)
	{
		auto& styles = instance()._styles;
		const std::string key = keyFrom(p_name);

		auto [iterator, inserted] = styles.try_emplace(key, spk::WidgetStyle::makeDefault());
		(void)inserted;

		return iterator->second;
	}

	void WidgetStyle::Collection::setStyle(std::string_view p_name, const spk::WidgetStyle& p_style)
	{
		spk::WidgetStyle& target = style(p_name);
		target = p_style;
		target.notifyEdition();
	}

}
