#pragma once

#include <array>

namespace spk
{
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
	};
}
