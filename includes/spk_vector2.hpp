#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "spk_approx_value.hpp"

namespace spk
{
	template <typename TType>
	concept vector2_value_type =
		std::is_arithmetic_v<TType>;

	template <vector2_value_type TType>
	struct IVector2
	{
		using value_type = TType;

		TType x{};
		TType y{};

		constexpr IVector2() = default;

		template <typename TX, typename TY>
			requires std::constructible_from<TType, TX> &&
					 std::constructible_from<TType, TY>
		constexpr IVector2(const TX& p_x, const TY& p_y) noexcept(
			std::is_nothrow_constructible_v<TType, TX> &&
			std::is_nothrow_constructible_v<TType, TY>) :
			x(static_cast<TType>(p_x)),
			y(static_cast<TType>(p_y))
		{
		}

		template <typename TOther>
			requires std::constructible_from<TType, TOther>
		explicit constexpr IVector2(const IVector2<TOther>& p_other) noexcept(std::is_nothrow_constructible_v<TType, TOther>) :
			x(static_cast<TType>(p_other.x)),
			y(static_cast<TType>(p_other.y))
		{
		}

		template <typename TOther>
			requires std::constructible_from<TType, TOther>
		explicit constexpr IVector2(const TOther& p_value) noexcept(std::is_nothrow_constructible_v<TType, TOther>) :
			x(static_cast<TType>(p_value)),
			y(static_cast<TType>(p_value))
		{
		}

		static const IVector2<TType> Zero;
		static const IVector2<TType> One;

		[[nodiscard]] std::string toString() const
		{
			std::ostringstream outputStream;
			outputStream << *this;
			return outputStream.str();
		}

		friend std::ostream& operator<<(std::ostream& p_outputStream, const IVector2<TType>& p_value)
		{
			p_outputStream << '(' << p_value.x << ", " << p_value.y << ')';
			return p_outputStream;
		}

		[[nodiscard]] constexpr bool operator==(const IVector2<TType>& p_other) const noexcept
		{
			return spk::ApproxValue(x) == p_other.x &&
				   spk::ApproxValue(y) == p_other.y;
		}

		[[nodiscard]] constexpr bool operator!=(const IVector2<TType>& p_other) const noexcept
		{
			return !(*this == p_other);
		}

		[[nodiscard]] constexpr IVector2<TType> operator+(const IVector2<TType>& p_other) const noexcept
		{
			return IVector2<TType>(x + p_other.x, y + p_other.y);
		}

		[[nodiscard]] constexpr IVector2<TType> operator-(const IVector2<TType>& p_other) const noexcept
		{
			return IVector2<TType>(x - p_other.x, y - p_other.y);
		}

		[[nodiscard]] constexpr IVector2<TType> operator*(const IVector2<TType>& p_other) const noexcept
		{
			return IVector2<TType>(x * p_other.x, y * p_other.y);
		}

		[[nodiscard]] IVector2<TType> operator/(const IVector2<TType>& p_other) const
		{
			if (spk::ApproxValue(p_other.x) == 0 ||
				spk::ApproxValue(p_other.y) == 0)
			{
				throw std::invalid_argument("spk::IVector2: division by zero vector component");
			}

			return IVector2<TType>(x / p_other.x, y / p_other.y);
		}

		[[nodiscard]] constexpr IVector2<TType> operator+(const TType& p_scalar) const noexcept
		{
			return IVector2<TType>(x + p_scalar, y + p_scalar);
		}

		[[nodiscard]] constexpr IVector2<TType> operator-(const TType& p_scalar) const noexcept
		{
			return IVector2<TType>(x - p_scalar, y - p_scalar);
		}

		[[nodiscard]] constexpr IVector2<TType> operator*(const TType& p_scalar) const noexcept
		{
			return IVector2<TType>(x * p_scalar, y * p_scalar);
		}

		[[nodiscard]] IVector2<TType> operator/(const TType& p_scalar) const
		{
			if (spk::ApproxValue(p_scalar) == 0)
			{
				throw std::invalid_argument("spk::IVector2: division by zero scalar");
			}

			return IVector2<TType>(x / p_scalar, y / p_scalar);
		}

		friend constexpr IVector2<TType> operator+(const TType& p_scalar, const IVector2<TType>& p_vector) noexcept
		{
			return p_vector + p_scalar;
		}

		friend constexpr IVector2<TType> operator-(const TType& p_scalar, const IVector2<TType>& p_vector) noexcept
		{
			return IVector2<TType>(p_scalar - p_vector.x, p_scalar - p_vector.y);
		}

		friend constexpr IVector2<TType> operator*(const TType& p_scalar, const IVector2<TType>& p_vector) noexcept
		{
			return p_vector * p_scalar;
		}

		friend IVector2<TType> operator/(const TType& p_scalar, const IVector2<TType>& p_vector)
		{
			if (spk::ApproxValue(p_vector.x) == 0 ||
				spk::ApproxValue(p_vector.y) == 0)
			{
				throw std::invalid_argument("spk::IVector2: division by zero vector component");
			}

			return IVector2<TType>(p_scalar / p_vector.x, p_scalar / p_vector.y);
		}

		[[nodiscard]] constexpr IVector2<TType> inverse() const noexcept
		{
			return IVector2<TType>(-x, -y);
		}

		[[nodiscard]] constexpr IVector2<TType> operator-() const noexcept
		{
			return inverse();
		}

		constexpr IVector2<TType>& operator+=(const IVector2<TType>& p_other) noexcept
		{
			x += p_other.x;
			y += p_other.y;
			return *this;
		}

		constexpr IVector2<TType>& operator-=(const IVector2<TType>& p_other) noexcept
		{
			x -= p_other.x;
			y -= p_other.y;
			return *this;
		}

		constexpr IVector2<TType>& operator*=(const IVector2<TType>& p_other) noexcept
		{
			x *= p_other.x;
			y *= p_other.y;
			return *this;
		}

		IVector2<TType>& operator/=(const IVector2<TType>& p_other)
		{
			if (spk::ApproxValue(p_other.x) == 0 ||
				spk::ApproxValue(p_other.y) == 0)
			{
				throw std::invalid_argument("spk::IVector2: division by zero vector component");
			}

			x /= p_other.x;
			y /= p_other.y;
			return *this;
		}

		constexpr IVector2<TType>& operator+=(const TType& p_scalar) noexcept
		{
			x += p_scalar;
			y += p_scalar;
			return *this;
		}

		constexpr IVector2<TType>& operator-=(const TType& p_scalar) noexcept
		{
			x -= p_scalar;
			y -= p_scalar;
			return *this;
		}

		constexpr IVector2<TType>& operator*=(const TType& p_scalar) noexcept
		{
			x *= p_scalar;
			y *= p_scalar;
			return *this;
		}

		IVector2<TType>& operator/=(const TType& p_scalar)
		{
			if (spk::ApproxValue(p_scalar) == 0)
			{
				throw std::invalid_argument("spk::IVector2: division by zero scalar");
			}

			x /= p_scalar;
			y /= p_scalar;
			return *this;
		}

		[[nodiscard]] constexpr bool isZero() const noexcept
		{
			return spk::ApproxValue(x) == 0 &&
				   spk::ApproxValue(y) == 0;
		}

		[[nodiscard]] float squaredLength() const noexcept
		{
			const float valueX = static_cast<float>(x);
			const float valueY = static_cast<float>(y);
			return (valueX * valueX) + (valueY * valueY);
		}

		[[nodiscard]] float length() const noexcept
		{
			return static_cast<float>(std::sqrt(static_cast<double>(squaredLength())));
		}

		[[nodiscard]] IVector2<float> normalized() const
		{
			const float squaredVectorLength = squaredLength();

			if (spk::ApproxValue(squaredVectorLength) == 0)
			{
				throw std::runtime_error("spk::IVector2: cannot normalize a zero-length vector");
			}

			const float inverseLength = 1.0f / static_cast<float>(std::sqrt(static_cast<double>(squaredVectorLength)));

			return IVector2<float>(
				static_cast<float>(x) * inverseLength,
				static_cast<float>(y) * inverseLength);
		}

		[[nodiscard]] float squaredDistance(const IVector2<TType>& p_other) const noexcept
		{
			return (*this - p_other).squaredLength();
		}

		[[nodiscard]] float distance(const IVector2<TType>& p_other) const noexcept
		{
			return (*this - p_other).length();
		}

		[[nodiscard]] float dot(const IVector2<TType>& p_other) const noexcept
		{
			return (static_cast<float>(x) * static_cast<float>(p_other.x)) +
				   (static_cast<float>(y) * static_cast<float>(p_other.y));
		}

		[[nodiscard]] float cross(const IVector2<TType>& p_other) const noexcept
		{
			return (static_cast<float>(x) * static_cast<float>(p_other.y)) -
				   (static_cast<float>(y) * static_cast<float>(p_other.x));
		}

		[[nodiscard]] constexpr IVector2<TType> perpendicular() const noexcept
		{
			return IVector2<TType>(-y, x);
		}

		[[nodiscard]] static IVector2<TType> lerp(const IVector2<TType>& p_from, const IVector2<TType>& p_to, const float p_alpha) noexcept
		{
			const float fromX = static_cast<float>(p_from.x);
			const float fromY = static_cast<float>(p_from.y);

			return IVector2<TType>(
				static_cast<TType>(fromX + ((static_cast<float>(p_to.x) - fromX) * p_alpha)),
				static_cast<TType>(fromY + ((static_cast<float>(p_to.y) - fromY) * p_alpha)));
		}

		[[nodiscard]] static constexpr IVector2<TType> min(const IVector2<TType>& p_left, const IVector2<TType>& p_right) noexcept
		{
			return IVector2<TType>(
				std::min(p_left.x, p_right.x),
				std::min(p_left.y, p_right.y));
		}

		template <typename... TOtherTypes>
			requires (sizeof...(TOtherTypes) > 0) &&
					 (std::same_as<TOtherTypes, IVector2<TType>> && ...)
		[[nodiscard]] static constexpr IVector2<TType> min(const IVector2<TType>& p_first, const IVector2<TType>& p_second, const TOtherTypes&... p_remainingValues) noexcept
		{
			return min(min(p_first, p_second), p_remainingValues...);
		}

		[[nodiscard]] static IVector2<TType> min(std::initializer_list<IVector2<TType>> p_values)
		{
			if (p_values.size() == 0)
			{
				throw std::invalid_argument("spk::IVector2::min requires at least one value");
			}

			IVector2<TType> result = *p_values.begin();

			for (auto iterator = std::next(p_values.begin()); iterator != p_values.end(); ++iterator)
			{
				result = min(result, *iterator);
			}

			return result;
		}

		[[nodiscard]] static constexpr IVector2<TType> max(const IVector2<TType>& p_left, const IVector2<TType>& p_right) noexcept
		{
			return IVector2<TType>(
				std::max(p_left.x, p_right.x),
				std::max(p_left.y, p_right.y));
		}

		template <typename... TOtherTypes>
			requires (sizeof...(TOtherTypes) > 0) &&
					 (std::same_as<TOtherTypes, IVector2<TType>> && ...)
		[[nodiscard]] static constexpr IVector2<TType> max(const IVector2<TType>& p_first, const IVector2<TType>& p_second, const TOtherTypes&... p_remainingValues) noexcept
		{
			return max(max(p_first, p_second), p_remainingValues...);
		}

		[[nodiscard]] static IVector2<TType> max(std::initializer_list<IVector2<TType>> p_values)
		{
			if (p_values.size() == 0)
			{
				throw std::invalid_argument("spk::IVector2::max requires at least one value");
			}

			IVector2<TType> result = *p_values.begin();

			for (auto iterator = std::next(p_values.begin()); iterator != p_values.end(); ++iterator)
			{
				result = max(result, *iterator);
			}

			return result;
		}

		[[nodiscard]] static constexpr IVector2<TType> clamp(
			const IVector2<TType>& p_value,
			const IVector2<TType>& p_boundMinimum,
			const IVector2<TType>& p_boundMaximum) noexcept
		{
			const IVector2<TType> low(
				std::min(p_boundMinimum.x, p_boundMaximum.x),
				std::min(p_boundMinimum.y, p_boundMaximum.y));

			const IVector2<TType> high(
				std::max(p_boundMinimum.x, p_boundMaximum.x),
				std::max(p_boundMinimum.y, p_boundMaximum.y));

			return IVector2<TType>(
				std::clamp(p_value.x, low.x, high.x),
				std::clamp(p_value.y, low.y, high.y));
		}

		[[nodiscard]] constexpr IVector2<TType> clamped(
			const IVector2<TType>& p_boundMinimum,
			const IVector2<TType>& p_boundMaximum) const noexcept
		{
			return clamp(*this, p_boundMinimum, p_boundMaximum);
		}

		[[nodiscard]] static constexpr bool isBetween(
			const IVector2<TType>& p_value,
			const IVector2<TType>& p_boundA,
			const IVector2<TType>& p_boundB) noexcept
		{
			const IVector2<TType> low(
				std::min(p_boundA.x, p_boundB.x),
				std::min(p_boundA.y, p_boundB.y));

			const IVector2<TType> high(
				std::max(p_boundA.x, p_boundB.x),
				std::max(p_boundA.y, p_boundB.y));

			return (p_value.x >= low.x && p_value.x <= high.x) &&
				   (p_value.y >= low.y && p_value.y <= high.y);
		}

		template <typename TOther>
			requires std::constructible_from<TOther, TType>
		explicit constexpr operator IVector2<TOther>() const noexcept(std::is_nothrow_constructible_v<TOther, TType>)
		{
			return IVector2<TOther>(
				static_cast<TOther>(x),
				static_cast<TOther>(y));
		}
	};

	template <vector2_value_type TType>
	inline const IVector2<TType> IVector2<TType>::Zero{};

	template <vector2_value_type TType>
	inline const IVector2<TType> IVector2<TType>::One{static_cast<TType>(1), static_cast<TType>(1)};

	using Vector2 = IVector2<float_t>;
	using Vector2Int = IVector2<std::int32_t>;
	using Vector2UInt = IVector2<std::uint32_t>;
}