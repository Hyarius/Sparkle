#pragma once

#include <numbers>

namespace spk
{
	[[nodiscard]] constexpr float degreeToRadian(const float p_degrees) noexcept
	{
		return p_degrees * std::numbers::pi_v<float> / 180.0f;
	}

	[[nodiscard]] constexpr float radianToDegree(const float p_radians) noexcept
	{
		return p_radians * 180.0f / std::numbers::pi_v<float>;
	}
}
