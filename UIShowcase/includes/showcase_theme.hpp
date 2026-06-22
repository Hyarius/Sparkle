#pragma once

#include <sparkle.hpp>

namespace showcase::theme
{
	// Semantic colors used across the showcase. These intentionally live in one place so a
	// future migration to a real spk::Palette / spk::Theme only has to touch this file.
	inline constexpr spk::Color background = spk::Color(0.12f, 0.13f, 0.16f, 1.0f);
	inline constexpr spk::Color primaryText = spk::Color(0.92f, 0.93f, 0.96f, 1.0f);
	inline constexpr spk::Color mutedText = spk::Color(0.66f, 0.69f, 0.74f, 1.0f);
	inline constexpr spk::Color accentText = spk::Color(0.45f, 0.74f, 1.0f, 1.0f);
	inline constexpr spk::Color textOutline = spk::Color(0.0f, 0.0f, 0.0f, 1.0f);

	// Builds a text style with the given glyph size and color, sharing the default font and a
	// thin dark outline for readability against the panel backgrounds.
	[[nodiscard]] inline spk::WidgetStyle textStyle(int p_glyphSize, const spk::Color &p_color)
	{
		spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
		style.setTextSize(spk::Font::Size(p_glyphSize, 1));
		style.setGlyphColor(p_color);
		style.setOutlineColor(textOutline);
		style.setTextPadding(spk::Vector2Int(0, 0));
		return style;
	}

	[[nodiscard]] inline spk::WidgetStyle headingStyle()
	{
		return textStyle(22, primaryText);
	}

	[[nodiscard]] inline spk::WidgetStyle bodyStyle()
	{
		return textStyle(16, primaryText);
	}

	[[nodiscard]] inline spk::WidgetStyle mutedStyle()
	{
		return textStyle(15, mutedText);
	}

	[[nodiscard]] inline spk::WidgetStyle accentStyle()
	{
		return textStyle(15, accentText);
	}

	// Button styles based on the default nine-slice backgrounds, but with a readable 1px
	// opaque outline (the bare Sparkle default uses a 0-width transparent outline, which
	// leaves button text hard to read against colored backgrounds).
	[[nodiscard]] inline spk::WidgetStyle buttonStyle(spk::WidgetStyle p_base, const spk::Color &p_glyphColor)
	{
		p_base.setTextSize(spk::Font::Size(15, 1));
		p_base.setGlyphColor(p_glyphColor);
		p_base.setOutlineColor(textOutline);
		p_base.setTextPadding(spk::Vector2Int(6, 2));
		return p_base;
	}

	[[nodiscard]] inline spk::WidgetStyle buttonReleasedStyle()
	{
		return buttonStyle(spk::WidgetStyle::makeDefault(), primaryText);
	}

	[[nodiscard]] inline spk::WidgetStyle buttonPressedStyle()
	{
		return buttonStyle(spk::WidgetStyle::makeDefaultPressed(), primaryText);
	}
}
