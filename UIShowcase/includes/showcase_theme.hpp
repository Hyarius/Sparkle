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

	namespace detail
	{
		// spk::WidgetStyle::makeDefault() / makeDefaultPressed() are expensive: each call
		// re-decodes the nine-slice and icon PNGs and re-parses the TTF font, allocating a
		// brand-new spk::Font (with its own GL atlas textures) every time. Calling them once
		// per label meant hundreds of redundant font/texture allocations, and tearing those
		// per-widget fonts down on page changes churned GL texture resources - which showed up
		// as the whole page appearing to restyle when navigating or focusing a widget.
		//
		// Loading the bases once and copying them shares a single font + sprite sheets across
		// every showcase style (a WidgetStyle copy shares the underlying shared_ptr resources),
		// so the atlases are created once and live for the whole program.
		[[nodiscard]] inline const spk::WidgetStyle &baseStyle()
		{
			static const spk::WidgetStyle base = spk::WidgetStyle::makeDefault();
			return base;
		}

		[[nodiscard]] inline const spk::WidgetStyle &basePressedStyle()
		{
			static const spk::WidgetStyle base = spk::WidgetStyle::makeDefaultPressed();
			return base;
		}
	}

	// Builds a text style with the given glyph size and color. No outline: a 1px outline baked
	// into the glyph atlas looks heavy and uneven at small sizes, so the showcase renders plain
	// glyphs (the bare Sparkle default already uses a 0-width transparent outline).
	[[nodiscard]] inline spk::WidgetStyle textStyle(int p_glyphSize, const spk::Color &p_color)
	{
		spk::WidgetStyle style = detail::baseStyle();
		style.setTextSize(spk::Font::Size(p_glyphSize, 0));
		style.setGlyphColor(p_color);
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

	// Button styles based on the default nine-slice backgrounds, sharing the cached font/sheets.
	[[nodiscard]] inline spk::WidgetStyle buttonStyle(spk::WidgetStyle p_base, const spk::Color &p_glyphColor)
	{
		p_base.setTextSize(spk::Font::Size(15, 0));
		p_base.setGlyphColor(p_glyphColor);
		p_base.setTextPadding(spk::Vector2Int(6, 2));
		return p_base;
	}

	[[nodiscard]] inline spk::WidgetStyle buttonReleasedStyle()
	{
		return buttonStyle(detail::baseStyle(), primaryText);
	}

	[[nodiscard]] inline spk::WidgetStyle buttonPressedStyle()
	{
		return buttonStyle(detail::basePressedStyle(), primaryText);
	}
}
