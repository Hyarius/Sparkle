#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace spk
{
	struct Color
	{
		enum class ChannelOrder
		{
			RGBA,
			ARGB,
			BGRA,
			ABGR
		};
		enum class Format
		{
			Decimal,
			Hexadecimal
		};
		float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

	private:
		static constexpr void _validateChannel(float p_value)
		{
			if (!std::isfinite(p_value) || p_value < 0.0f || p_value > 1.0f)
			{
				throw std::invalid_argument("spk::Color: channels must be finite values in [0, 1]");
			}
		}
		[[nodiscard]] static constexpr Color _fromUnchecked(float p_red, float p_green, float p_blue, float p_alpha) noexcept
		{
			Color result;
			result.r = p_red;
			result.g = p_green;
			result.b = p_blue;
			result.a = p_alpha;
			return result;
		}
		[[nodiscard]] static constexpr std::array<float, 4> _ordered(float p_first, float p_second, float p_third, float p_fourth, ChannelOrder p_order) noexcept
		{
			switch (p_order)
			{
			case ChannelOrder::RGBA:
				return {p_first, p_second, p_third, p_fourth};
			case ChannelOrder::ARGB:
				return {p_second, p_third, p_fourth, p_first};
			case ChannelOrder::BGRA:
				return {p_third, p_second, p_first, p_fourth};
			case ChannelOrder::ABGR:
				return {p_fourth, p_third, p_second, p_first};
			}
			return {};
		}
		[[nodiscard]] constexpr std::array<float, 4> _ordered(ChannelOrder p_order) const noexcept
		{
			switch (p_order)
			{
			case ChannelOrder::RGBA:
				return {r, g, b, a};
			case ChannelOrder::ARGB:
				return {a, r, g, b};
			case ChannelOrder::BGRA:
				return {b, g, r, a};
			case ChannelOrder::ABGR:
				return {a, b, g, r};
			}
			return {};
		}
		[[nodiscard]] static constexpr const char *_orderName(ChannelOrder p_order) noexcept
		{
			switch (p_order)
			{
			case ChannelOrder::RGBA:
				return "RGBA";
			case ChannelOrder::ARGB:
				return "ARGB";
			case ChannelOrder::BGRA:
				return "BGRA";
			case ChannelOrder::ABGR:
				return "ABGR";
			}
			return "RGBA";
		}
		[[nodiscard]] static constexpr int _hexDigit(char p_character) noexcept
		{
			return p_character >= '0' && p_character <= '9' ? p_character - '0' : p_character >= 'a' && p_character <= 'f' ? 10 + p_character - 'a'
																			  : p_character >= 'A' && p_character <= 'F'   ? 10 + p_character - 'A'
																														   : -1;
		}
		[[nodiscard]] static unsigned _hexChannel(float p_value) noexcept
		{
			if (std::isnan(p_value) || p_value == -INFINITY)
			{
				return 0;
			}
			if (p_value == INFINITY)
			{
				return 255;
			}
			return static_cast<unsigned>(std::lround(std::clamp(p_value, 0.0f, 1.0f) * 255.0f));
		}
		void _writeToStream(std::ostream &p_stream, Format p_format, ChannelOrder p_order, bool p_includeAlpha = true) const
		{
			const auto values = _ordered(p_order);
			if (p_format == Format::Decimal)
			{
				p_stream << _orderName(p_order) << '(' << values[0] << ", " << values[1] << ", " << values[2] << ", " << values[3] << ')';
				return;
			}
			const auto flags = p_stream.flags();
			const char fill = p_stream.fill();
			p_stream << '#' << std::uppercase << std::hex << std::setfill('0');
			for (std::size_t index = 0; index < (p_includeAlpha ? 4u : 3u); ++index)
			{
				p_stream << std::setw(2) << _hexChannel(values[index]);
			}
			p_stream.flags(flags);
			p_stream.fill(fill);
		}

	public:
		constexpr Color() = default;
		constexpr Color(float p_first, float p_second, float p_third, ChannelOrder p_order = ChannelOrder::RGBA)
		{
			_validateChannel(p_first);
			_validateChannel(p_second);
			_validateChannel(p_third);
			const auto values = p_order == ChannelOrder::RGBA || p_order == ChannelOrder::ARGB ? _ordered(p_first, p_second, p_third, 1.0f, ChannelOrder::RGBA) : _ordered(p_first, p_second, p_third, 1.0f, ChannelOrder::BGRA);
			r = values[0];
			g = values[1];
			b = values[2];
			a = 1.0f;
		}
		constexpr Color(float p_first, float p_second, float p_third, float p_fourth, ChannelOrder p_order = ChannelOrder::RGBA)
		{
			_validateChannel(p_first);
			_validateChannel(p_second);
			_validateChannel(p_third);
			_validateChannel(p_fourth);
			const auto values = _ordered(p_first, p_second, p_third, p_fourth, p_order);
			r = values[0];
			g = values[1];
			b = values[2];
			a = values[3];
		}
		[[nodiscard]] static constexpr Color fromHex(std::string_view p_hexCode, ChannelOrder p_order = ChannelOrder::RGBA)
		{
			if ((p_hexCode.size() != 7 && p_hexCode.size() != 9) || p_hexCode.empty() || p_hexCode.front() != '#')
			{
				throw std::invalid_argument("spk::Color: invalid hexadecimal color");
			}
			auto byteAt = [&](std::size_t p_index) constexpr -> float {
				const int high = _hexDigit(p_hexCode[p_index]);
				const int low = _hexDigit(p_hexCode[p_index + 1]);
				if (high < 0 || low < 0)
				{
					throw std::invalid_argument("spk::Color: invalid hexadecimal color");
				}
				return static_cast<float>((high << 4) | low) / 255.0f;
			};
			const float first = byteAt(1), second = byteAt(3), third = byteAt(5);
			return p_hexCode.size() == 9 ? Color(first, second, third, byteAt(7), p_order) : Color(first, second, third, p_order);
		}
		[[nodiscard]] constexpr std::array<float, 4> values() const noexcept
		{
			return {r, g, b, a};
		}
		[[nodiscard]] constexpr float &operator[](std::size_t p_index)
		{
			switch (p_index)
			{
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
			default:
				throw std::out_of_range("spk::Color: channel index out of range");
			}
		}
		[[nodiscard]] constexpr const float &operator[](std::size_t p_index) const
		{
			switch (p_index)
			{
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
			default:
				throw std::out_of_range("spk::Color: channel index out of range");
			}
		}
		[[nodiscard]] bool isNormalized() const noexcept
		{
			return std::isfinite(r) && std::isfinite(g) && std::isfinite(b) && std::isfinite(a) && r >= 0 && r <= 1 && g >= 0 && g <= 1 && b >= 0 && b <= 1 && a >= 0 && a <= 1;
		}
		constexpr Color &clamp() noexcept
		{
			auto clampChannel = [](float p_value) constexpr noexcept {
				return std::isnan(p_value) || p_value < 0.0f ? 0.0f : p_value > 1.0f ? 1.0f
																					 : p_value;
			};
			r = clampChannel(r);
			g = clampChannel(g);
			b = clampChannel(b);
			a = clampChannel(a);
			return *this;
		}
		[[nodiscard]] constexpr Color clamped() const noexcept
		{
			Color result = *this;
			return result.clamp();
		}
		[[nodiscard]] std::string toString(Format p_format = Format::Decimal, ChannelOrder p_order = ChannelOrder::RGBA) const
		{
			std::ostringstream stream;
			_writeToStream(stream, p_format, p_order);
			return stream.str();
		}
		constexpr Color &operator+=(const Color &p_other) noexcept
		{
			r += p_other.r;
			g += p_other.g;
			b += p_other.b;
			a += p_other.a;
			return *this;
		}
		constexpr Color &operator-=(const Color &p_other) noexcept
		{
			r -= p_other.r;
			g -= p_other.g;
			b -= p_other.b;
			a -= p_other.a;
			return *this;
		}
		constexpr Color &operator*=(const Color &p_other) noexcept
		{
			r *= p_other.r;
			g *= p_other.g;
			b *= p_other.b;
			a *= p_other.a;
			return *this;
		}
		constexpr Color &operator*=(float p_scalar) noexcept
		{
			r *= p_scalar;
			g *= p_scalar;
			b *= p_scalar;
			a *= p_scalar;
			return *this;
		}
		constexpr Color &operator/=(float p_scalar)
		{
			if (p_scalar == 0.0f)
			{
				throw std::domain_error("spk::Color: division by zero");
			}
			return *this *= 1.0f / p_scalar;
		}
		friend constexpr Color operator+(Color p_left, const Color &p_right) noexcept
		{
			return p_left += p_right;
		}
		friend constexpr Color operator-(Color p_left, const Color &p_right) noexcept
		{
			return p_left -= p_right;
		}
		friend constexpr Color operator*(Color p_left, const Color &p_right) noexcept
		{
			return p_left *= p_right;
		}
		friend constexpr Color operator*(Color p_left, float p_right) noexcept
		{
			return p_left *= p_right;
		}
		friend constexpr Color operator*(float p_left, Color p_right) noexcept
		{
			return p_right *= p_left;
		}
		friend constexpr Color operator/(Color p_left, float p_right)
		{
			return p_left /= p_right;
		}
		[[nodiscard]] static constexpr Color lerp(const Color &p_from, const Color &p_to, float p_ratio) noexcept
		{
			return _fromUnchecked(p_from.r + (p_to.r - p_from.r) * p_ratio, p_from.g + (p_to.g - p_from.g) * p_ratio, p_from.b + (p_to.b - p_from.b) * p_ratio, p_from.a + (p_to.a - p_from.a) * p_ratio);
		}
		[[nodiscard]] constexpr bool operator==(const Color &) const noexcept = default;
		constexpr bool operator!=(const Color &) const noexcept = default;
		static const Color White, Black, Transparent, Red, Green, Blue;
		friend std::ostream &operator<<(std::ostream &, const Color &);
		friend struct ColorHexManipulator;
	};

	using ChannelOrder = Color::ChannelOrder;
	struct ColorHexManipulator
	{
		ChannelOrder order = ChannelOrder::RGBA;
		bool includeAlpha = true;
		[[nodiscard]] constexpr ColorHexManipulator operator()(ChannelOrder p_order = ChannelOrder::RGBA, bool p_includeAlpha = true) const noexcept
		{
			return {p_order, p_includeAlpha};
		}
	};
	struct ColorComponentManipulator
	{
	};
	inline constexpr ColorHexManipulator toHex{};
	inline constexpr ColorComponentManipulator toDecimal{};
	inline int _colorFormatIndex()
	{
		static const int value = std::ios_base::xalloc();
		return value;
	}
	inline std::ostream &operator<<(std::ostream &p_stream, ColorHexManipulator p_manipulator)
	{
		p_stream.iword(_colorFormatIndex()) = 1 + static_cast<long>(p_manipulator.order) * 2 + (p_manipulator.includeAlpha ? 1 : 0);
		return p_stream;
	}
	inline std::ostream &operator<<(std::ostream &p_stream, ColorComponentManipulator)
	{
		p_stream.iword(_colorFormatIndex()) = 0;
		return p_stream;
	}
	inline std::ostream &operator<<(std::ostream &p_stream, const Color &p_color)
	{
		const long setting = p_stream.iword(_colorFormatIndex());
		if (setting == 0)
		{
			p_color._writeToStream(p_stream, Color::Format::Decimal, ChannelOrder::RGBA);
		}
		else
		{
			const long value = setting - 1;
			p_color._writeToStream(p_stream, Color::Format::Hexadecimal, static_cast<ChannelOrder>(value / 2), (value % 2) != 0);
		}
		return p_stream;
	}
	inline constexpr Color Color::White(1, 1, 1, 1), Color::Black(0, 0, 0, 1), Color::Transparent(0, 0, 0, 0), Color::Red(1, 0, 0, 1), Color::Green(0, 1, 0, 1), Color::Blue(0, 0, 1, 1);
}
