#pragma once

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

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

		// Parses "#RRGGBB" (alpha forced to FF) or "#RRGGBBAA"; throws std::invalid_argument
		// on any other shape or non-hexadecimal digit.
		constexpr explicit Color(std::string_view p_hexCode)
		{
			if ((p_hexCode.size() != 7 && p_hexCode.size() != 9) || p_hexCode.front() != '#')
			{
				throw std::invalid_argument("Color hex code must look like \"#RRGGBB\" or \"#RRGGBBAA\"");
			}
			const auto digit = [](char p_character) -> int {
				if (p_character >= '0' && p_character <= '9')
				{
					return p_character - '0';
				}
				if (p_character >= 'a' && p_character <= 'f')
				{
					return 10 + (p_character - 'a');
				}
				if (p_character >= 'A' && p_character <= 'F')
				{
					return 10 + (p_character - 'A');
				}
				throw std::invalid_argument("Color hex code contains a non-hexadecimal digit");
			};
			const auto channel = [&](std::size_t p_offset) {
				return static_cast<float>(digit(p_hexCode[p_offset]) * 16 + digit(p_hexCode[p_offset + 1])) / 255.0f;
			};
			r = channel(1);
			g = channel(3);
			b = channel(5);
			a = p_hexCode.size() == 9 ? channel(7) : 1.0f;
		}

		[[nodiscard]] constexpr std::array<float, 4> values() const noexcept
		{
			return {r, g, b, a};
		}

		friend std::ostream &operator<<(std::ostream &p_outputStream, const Color &p_color)
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
