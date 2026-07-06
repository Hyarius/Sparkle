#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "structures/math/spk_approx_value.hpp"
#include "structures/math/spk_vector2.hpp"
#include "type/spk_concepts.hpp"

namespace spk
{
	template <spk::arithmetic_value TType>
	struct IVector3
	{
		using value_type = TType;

		TType x{};
		TType y{};
		TType z{};

		constexpr IVector3() = default;

		template <typename TX, typename TY, typename TZ>
			requires std::constructible_from<TType, TX> &&
						 std::constructible_from<TType, TY> &&
						 std::constructible_from<TType, TZ>
		constexpr IVector3(const TX &p_x, const TY &p_y, const TZ &p_z) noexcept(
			std::is_nothrow_constructible_v<TType, TX> &&
			std::is_nothrow_constructible_v<TType, TY> &&
			std::is_nothrow_constructible_v<TType, TZ>) :
			x(static_cast<TType>(p_x)),
			y(static_cast<TType>(p_y)),
			z(static_cast<TType>(p_z))
		{
		}

		template <typename TOther>
			requires std::constructible_from<TType, TOther>
		explicit constexpr IVector3(const IVector3<TOther> &p_other) noexcept(std::is_nothrow_constructible_v<TType, TOther>) :
			x(static_cast<TType>(p_other.x)),
			y(static_cast<TType>(p_other.y)),
			z(static_cast<TType>(p_other.z))
		{
		}

		template <typename TOther>
			requires std::constructible_from<TType, TOther>
		explicit constexpr IVector3(const TOther &p_value) noexcept(std::is_nothrow_constructible_v<TType, TOther>) :
			x(static_cast<TType>(p_value)),
			y(static_cast<TType>(p_value)),
			z(static_cast<TType>(p_value))
		{
		}

		template <typename TOther, typename TZ>
			requires std::constructible_from<TType, TOther> &&
						 std::constructible_from<TType, TZ>
		constexpr IVector3(const IVector2<TOther> &p_xy, const TZ &p_z) noexcept(
			std::is_nothrow_constructible_v<TType, TOther> &&
			std::is_nothrow_constructible_v<TType, TZ>) :
			x(static_cast<TType>(p_xy.x)),
			y(static_cast<TType>(p_xy.y)),
			z(static_cast<TType>(p_z))
		{
		}

		static const IVector3<TType> Zero;
		static const IVector3<TType> One;

		[[nodiscard]] std::string toString() const
		{
			std::ostringstream outputStream;
			outputStream << *this;
			return outputStream.str();
		}

		friend std::ostream &operator<<(std::ostream &p_outputStream, const IVector3<TType> &p_value)
		{
			p_outputStream << '(' << p_value.x << ", " << p_value.y << ", " << p_value.z << ')';
			return p_outputStream;
		}

		[[nodiscard]] constexpr bool operator==(const IVector3<TType> &p_other) const noexcept
		{
			return spk::ApproxValue(x) == p_other.x &&
				   spk::ApproxValue(y) == p_other.y &&
				   spk::ApproxValue(z) == p_other.z;
		}

		[[nodiscard]] constexpr bool operator!=(const IVector3<TType> &p_other) const noexcept
		{
			return !(*this == p_other);
		}

		[[nodiscard]] constexpr IVector3<TType> operator+(const IVector3<TType> &p_other) const noexcept
		{
			return IVector3<TType>(x + p_other.x, y + p_other.y, z + p_other.z);
		}

		[[nodiscard]] constexpr IVector3<TType> operator-(const IVector3<TType> &p_other) const noexcept
		{
			return IVector3<TType>(x - p_other.x, y - p_other.y, z - p_other.z);
		}

		[[nodiscard]] constexpr IVector3<TType> operator*(const IVector3<TType> &p_other) const noexcept
		{
			return IVector3<TType>(x * p_other.x, y * p_other.y, z * p_other.z);
		}

		[[nodiscard]] IVector3<TType> operator/(const IVector3<TType> &p_other) const
		{
			if (spk::ApproxValue(p_other.x) == 0 ||
				spk::ApproxValue(p_other.y) == 0 ||
				spk::ApproxValue(p_other.z) == 0)
			{
				throw std::invalid_argument("spk::IVector3: division by zero vector component");
			}

			return IVector3<TType>(x / p_other.x, y / p_other.y, z / p_other.z);
		}

		[[nodiscard]] constexpr IVector3<TType> operator+(const TType &p_scalar) const noexcept
		{
			return IVector3<TType>(x + p_scalar, y + p_scalar, z + p_scalar);
		}

		[[nodiscard]] constexpr IVector3<TType> operator-(const TType &p_scalar) const noexcept
		{
			return IVector3<TType>(x - p_scalar, y - p_scalar, z - p_scalar);
		}

		[[nodiscard]] constexpr IVector3<TType> operator*(const TType &p_scalar) const noexcept
		{
			return IVector3<TType>(x * p_scalar, y * p_scalar, z * p_scalar);
		}

		[[nodiscard]] IVector3<TType> operator/(const TType &p_scalar) const
		{
			if (spk::ApproxValue(p_scalar) == 0)
			{
				throw std::invalid_argument("spk::IVector3: division by zero scalar");
			}

			return IVector3<TType>(x / p_scalar, y / p_scalar, z / p_scalar);
		}

		friend constexpr IVector3<TType> operator+(const TType &p_scalar, const IVector3<TType> &p_vector) noexcept
		{
			return p_vector + p_scalar;
		}

		friend constexpr IVector3<TType> operator-(const TType &p_scalar, const IVector3<TType> &p_vector) noexcept
		{
			return IVector3<TType>(p_scalar - p_vector.x, p_scalar - p_vector.y, p_scalar - p_vector.z);
		}

		friend constexpr IVector3<TType> operator*(const TType &p_scalar, const IVector3<TType> &p_vector) noexcept
		{
			return p_vector * p_scalar;
		}

		friend IVector3<TType> operator/(const TType &p_scalar, const IVector3<TType> &p_vector)
		{
			if (spk::ApproxValue(p_vector.x) == 0 ||
				spk::ApproxValue(p_vector.y) == 0 ||
				spk::ApproxValue(p_vector.z) == 0)
			{
				throw std::invalid_argument("spk::IVector3: division by zero vector component");
			}

			return IVector3<TType>(p_scalar / p_vector.x, p_scalar / p_vector.y, p_scalar / p_vector.z);
		}

		[[nodiscard]] constexpr IVector3<TType> inverse() const noexcept
		{
			return IVector3<TType>(-x, -y, -z);
		}

		[[nodiscard]] constexpr IVector3<TType> operator-() const noexcept
		{
			return inverse();
		}

		constexpr IVector3<TType> &operator+=(const IVector3<TType> &p_other) noexcept
		{
			x += p_other.x;
			y += p_other.y;
			z += p_other.z;
			return *this;
		}

		constexpr IVector3<TType> &operator-=(const IVector3<TType> &p_other) noexcept
		{
			x -= p_other.x;
			y -= p_other.y;
			z -= p_other.z;
			return *this;
		}

		constexpr IVector3<TType> &operator*=(const IVector3<TType> &p_other) noexcept
		{
			x *= p_other.x;
			y *= p_other.y;
			z *= p_other.z;
			return *this;
		}

		IVector3<TType> &operator/=(const IVector3<TType> &p_other)
		{
			if (spk::ApproxValue(p_other.x) == 0 ||
				spk::ApproxValue(p_other.y) == 0 ||
				spk::ApproxValue(p_other.z) == 0)
			{
				throw std::invalid_argument("spk::IVector3: division by zero vector component");
			}

			x /= p_other.x;
			y /= p_other.y;
			z /= p_other.z;
			return *this;
		}

		constexpr IVector3<TType> &operator+=(const TType &p_scalar) noexcept
		{
			x += p_scalar;
			y += p_scalar;
			z += p_scalar;
			return *this;
		}

		constexpr IVector3<TType> &operator-=(const TType &p_scalar) noexcept
		{
			x -= p_scalar;
			y -= p_scalar;
			z -= p_scalar;
			return *this;
		}

		constexpr IVector3<TType> &operator*=(const TType &p_scalar) noexcept
		{
			x *= p_scalar;
			y *= p_scalar;
			z *= p_scalar;
			return *this;
		}

		IVector3<TType> &operator/=(const TType &p_scalar)
		{
			if (spk::ApproxValue(p_scalar) == 0)
			{
				throw std::invalid_argument("spk::IVector3: division by zero scalar");
			}

			x /= p_scalar;
			y /= p_scalar;
			z /= p_scalar;
			return *this;
		}

		[[nodiscard]] constexpr bool isZero() const noexcept
		{
			return spk::ApproxValue(x) == 0 &&
				   spk::ApproxValue(y) == 0 &&
				   spk::ApproxValue(z) == 0;
		}

		[[nodiscard]] float squaredLength() const noexcept
		{
			const float valueX = static_cast<float>(x);
			const float valueY = static_cast<float>(y);
			const float valueZ = static_cast<float>(z);
			return (valueX * valueX) + (valueY * valueY) + (valueZ * valueZ);
		}

		[[nodiscard]] float length() const noexcept
		{
			return static_cast<float>(std::sqrt(static_cast<double>(squaredLength())));
		}

		[[nodiscard]] IVector3<float> normalized() const
		{
			const float squaredVectorLength = squaredLength();

			if (spk::ApproxValue(squaredVectorLength) == 0)
			{
				throw std::runtime_error("spk::IVector3: cannot normalize a zero-length vector");
			}

			const float inverseLength = 1.0f / static_cast<float>(std::sqrt(static_cast<double>(squaredVectorLength)));

			return IVector3<float>(
				static_cast<float>(x) * inverseLength,
				static_cast<float>(y) * inverseLength,
				static_cast<float>(z) * inverseLength);
		}

		[[nodiscard]] float squaredDistance(const IVector3<TType> &p_other) const noexcept
		{
			return (*this - p_other).squaredLength();
		}

		[[nodiscard]] float distance(const IVector3<TType> &p_other) const noexcept
		{
			return (*this - p_other).length();
		}

		[[nodiscard]] float dot(const IVector3<TType> &p_other) const noexcept
		{
			return (static_cast<float>(x) * static_cast<float>(p_other.x)) +
				   (static_cast<float>(y) * static_cast<float>(p_other.y)) +
				   (static_cast<float>(z) * static_cast<float>(p_other.z));
		}

		[[nodiscard]] constexpr IVector3<TType> cross(const IVector3<TType> &p_other) const noexcept
		{
			return IVector3<TType>(
				y * p_other.z - z * p_other.y,
				z * p_other.x - x * p_other.z,
				x * p_other.y - y * p_other.x);
		}

		[[nodiscard]] constexpr IVector2<TType> xy() const noexcept
		{
			return IVector2<TType>(x, y);
		}

		[[nodiscard]] constexpr IVector2<TType> xz() const noexcept
		{
			return IVector2<TType>(x, z);
		}

		[[nodiscard]] constexpr IVector2<TType> yz() const noexcept
		{
			return IVector2<TType>(y, z);
		}

		[[nodiscard]] static IVector3<TType> lerp(const IVector3<TType> &p_from, const IVector3<TType> &p_to, const float p_alpha) noexcept
		{
			const float fromX = static_cast<float>(p_from.x);
			const float fromY = static_cast<float>(p_from.y);
			const float fromZ = static_cast<float>(p_from.z);

			return IVector3<TType>(
				static_cast<TType>(fromX + ((static_cast<float>(p_to.x) - fromX) * p_alpha)),
				static_cast<TType>(fromY + ((static_cast<float>(p_to.y) - fromY) * p_alpha)),
				static_cast<TType>(fromZ + ((static_cast<float>(p_to.z) - fromZ) * p_alpha)));
		}

		[[nodiscard]] static constexpr IVector3<TType> min(const IVector3<TType> &p_left, const IVector3<TType> &p_right) noexcept
		{
			return IVector3<TType>(
				std::min(p_left.x, p_right.x),
				std::min(p_left.y, p_right.y),
				std::min(p_left.z, p_right.z));
		}

		template <typename... TOtherTypes>
			requires(sizeof...(TOtherTypes) > 0) &&
					(std::same_as<TOtherTypes, IVector3<TType>> && ...)
		[[nodiscard]] static constexpr IVector3<TType> min(const IVector3<TType> &p_first, const IVector3<TType> &p_second, const TOtherTypes &...p_remainingValues) noexcept
		{
			return min(min(p_first, p_second), p_remainingValues...);
		}

		[[nodiscard]] static IVector3<TType> min(std::initializer_list<IVector3<TType>> p_values)
		{
			if (p_values.size() == 0)
			{
				throw std::invalid_argument("spk::IVector3::min requires at least one value");
			}

			IVector3<TType> result = *p_values.begin();
			for (auto iterator = std::next(p_values.begin()); iterator != p_values.end(); ++iterator)
			{
				result = min(result, *iterator);
			}
			return result;
		}

		[[nodiscard]] static constexpr IVector3<TType> max(const IVector3<TType> &p_left, const IVector3<TType> &p_right) noexcept
		{
			return IVector3<TType>(
				std::max(p_left.x, p_right.x),
				std::max(p_left.y, p_right.y),
				std::max(p_left.z, p_right.z));
		}

		template <typename... TOtherTypes>
			requires(sizeof...(TOtherTypes) > 0) &&
					(std::same_as<TOtherTypes, IVector3<TType>> && ...)
		[[nodiscard]] static constexpr IVector3<TType> max(const IVector3<TType> &p_first, const IVector3<TType> &p_second, const TOtherTypes &...p_remainingValues) noexcept
		{
			return max(max(p_first, p_second), p_remainingValues...);
		}

		[[nodiscard]] static IVector3<TType> max(std::initializer_list<IVector3<TType>> p_values)
		{
			if (p_values.size() == 0)
			{
				throw std::invalid_argument("spk::IVector3::max requires at least one value");
			}

			IVector3<TType> result = *p_values.begin();
			for (auto iterator = std::next(p_values.begin()); iterator != p_values.end(); ++iterator)
			{
				result = max(result, *iterator);
			}
			return result;
		}

		[[nodiscard]] static constexpr IVector3<TType> clamp(
			const IVector3<TType> &p_value,
			const IVector3<TType> &p_boundMinimum,
			const IVector3<TType> &p_boundMaximum) noexcept
		{
			const IVector3<TType> low = min(p_boundMinimum, p_boundMaximum);
			const IVector3<TType> high = max(p_boundMinimum, p_boundMaximum);

			return IVector3<TType>(
				std::clamp(p_value.x, low.x, high.x),
				std::clamp(p_value.y, low.y, high.y),
				std::clamp(p_value.z, low.z, high.z));
		}

		[[nodiscard]] constexpr IVector3<TType> clamped(
			const IVector3<TType> &p_boundMinimum,
			const IVector3<TType> &p_boundMaximum) const noexcept
		{
			return clamp(*this, p_boundMinimum, p_boundMaximum);
		}

		[[nodiscard]] static constexpr bool isBetween(
			const IVector3<TType> &p_value,
			const IVector3<TType> &p_boundA,
			const IVector3<TType> &p_boundB) noexcept
		{
			const IVector3<TType> low = min(p_boundA, p_boundB);
			const IVector3<TType> high = max(p_boundA, p_boundB);

			return (p_value.x >= low.x && p_value.x <= high.x) &&
				   (p_value.y >= low.y && p_value.y <= high.y) &&
				   (p_value.z >= low.z && p_value.z <= high.z);
		}

		template <typename TOther>
			requires std::constructible_from<TOther, TType>
		explicit constexpr operator IVector3<TOther>() const noexcept(std::is_nothrow_constructible_v<TOther, TType>)
		{
			return IVector3<TOther>(
				static_cast<TOther>(x),
				static_cast<TOther>(y),
				static_cast<TOther>(z));
		}
	};

	template <spk::arithmetic_value TType>
	inline const IVector3<TType> IVector3<TType>::Zero{};

	template <spk::arithmetic_value TType>
	inline const IVector3<TType> IVector3<TType>::One{static_cast<TType>(1), static_cast<TType>(1), static_cast<TType>(1)};

	using Vector3 = IVector3<float>;
	using Vector3Int = IVector3<std::int32_t>;
	using Vector3UInt = IVector3<std::uint32_t>;
}

namespace std
{
	template <typename TType>
	struct hash<spk::IVector3<TType>>
	{
		std::size_t operator()(const spk::IVector3<TType> &p_value) const noexcept
		{
			std::size_t h = std::hash<TType>{}(p_value.x);
			h ^= std::hash<TType>{}(p_value.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<TType>{}(p_value.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
			return h;
		}
	};
}
