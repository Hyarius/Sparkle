#pragma once

#include <array>
#include <iostream>
#include <sstream>

namespace spk
{
	/**
	 * @brief RGBA color stored as four normalized floating-point channels.
	 *
	 * Use `Color` whenever a widget, mesh, or render command needs a tint or clear color. Components are expressed in the common 0.0 to
	 * 1.0 range, where alpha controls opacity.
	 *
	 * @code{.cpp}
	 * spk::Color warning = {1.0f, 0.75f, 0.0f, 1.0f};
	 * auto values = warning.values();
	 * std::string debugText = warning.toString();
	 * @endcode
	 *
	 * @see spk::ColorMesh2D
	 * @see spk::ColorRectangleRenderCommand
	 * @see spk::WidgetStyle
	 */
	struct Color
	{
		float r = 1.0f;
		float g = 1.0f;
		float b = 1.0f;
		float a = 1.0f;

		constexpr Color() = default;
		constexpr Color(float p_red, float p_green, float p_blue, float p_alpha = 1.0f) :
			r(p_red),
			g(p_green),
			b(p_blue),
			a(p_alpha)
		{
		}

		[[nodiscard]] constexpr std::array<float, 4> values() const noexcept
		{
			return {r, g, b, a};
		}

		friend std::ostream& operator<<(std::ostream& p_outputStream, const Color& p_color)
		{
			p_outputStream << "[" << p_color.r << " - " << p_color.g << " - " << p_color.b << " - " << p_color.a << "]";
			return p_outputStream;
		}

		[[nodiscard]] std::string toString() const
		{
			std::ostringstream outputStream;
			outputStream << *this;
			return outputStream.str();
		}
	};
}
