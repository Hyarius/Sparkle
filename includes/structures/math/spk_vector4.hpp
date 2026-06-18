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

#if defined(__SSE__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#	include <xmmintrin.h>
#	define SPK_VECTOR4_HAS_SSE 1
#else
#	define SPK_VECTOR4_HAS_SSE 0
#endif

#include "structures/math/spk_approx_value.hpp"
#include "type/spk_concepts.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	template <spk::arithmetic_value TType>
	struct IVector4
	{
		using value_type = TType;

		TType x{};
		TType y{};
		TType z{};
		TType w{};

#if SPK_VECTOR4_HAS_SSE
	private:
		[[nodiscard]] __m128 _simdValue() const noexcept
			requires std::same_as<TType, float>
		{
			return _mm_setr_ps(x, y, z, w);
		}

		[[nodiscard]] static IVector4<TType> _fromSimd(const __m128 p_value) noexcept
			requires std::same_as<TType, float>
		{
			alignas(16) float values[4];
			_mm_store_ps(values, p_value);
			return {values[0], values[1], values[2], values[3]};
		}

	public:
#endif

		constexpr IVector4() = default;

		template <typename TX, typename TY, typename TZ, typename TW>
			requires std::constructible_from<TType, TX> && std::constructible_from<TType, TY> && std::constructible_from<TType, TZ> &&
						 std::constructible_from<TType, TW>
		constexpr IVector4(const TX& p_x, const TY& p_y, const TZ& p_z, const TW& p_w) noexcept(
			std::is_nothrow_constructible_v<TType, TX> && std::is_nothrow_constructible_v<TType, TY> && std::is_nothrow_constructible_v<TType, TZ> &&
			std::is_nothrow_constructible_v<TType, TW>) :
			x(static_cast<TType>(p_x)),
			y(static_cast<TType>(p_y)),
			z(static_cast<TType>(p_z)),
			w(static_cast<TType>(p_w))
		{
		}

		template <typename TOther>
			requires std::constructible_from<TType, TOther>
		explicit constexpr IVector4(const IVector4<TOther>& p_other) noexcept(std::is_nothrow_constructible_v<TType, TOther>) :
			x(static_cast<TType>(p_other.x)),
			y(static_cast<TType>(p_other.y)),
			z(static_cast<TType>(p_other.z)),
			w(static_cast<TType>(p_other.w))
		{
		}

		template <typename TOther>
			requires std::constructible_from<TType, TOther>
		explicit constexpr IVector4(const TOther& p_value) noexcept(std::is_nothrow_constructible_v<TType, TOther>) :
			x(static_cast<TType>(p_value)),
			y(static_cast<TType>(p_value)),
			z(static_cast<TType>(p_value)),
			w(static_cast<TType>(p_value))
		{
		}

		template <typename TOther, typename TW>
			requires std::constructible_from<TType, TOther> && std::constructible_from<TType, TW>
		constexpr IVector4(const IVector3<TOther>& p_xyz, const TW& p_w) noexcept(
			std::is_nothrow_constructible_v<TType, TOther> && std::is_nothrow_constructible_v<TType, TW>) :
			x(static_cast<TType>(p_xyz.x)),
			y(static_cast<TType>(p_xyz.y)),
			z(static_cast<TType>(p_xyz.z)),
			w(static_cast<TType>(p_w))
		{
		}

		template <typename TOther, typename TZ, typename TW>
			requires std::constructible_from<TType, TOther> && std::constructible_from<TType, TZ> && std::constructible_from<TType, TW>
		constexpr IVector4(const IVector2<TOther>& p_xy, const TZ& p_z, const TW& p_w) noexcept(
			std::is_nothrow_constructible_v<TType, TOther> && std::is_nothrow_constructible_v<TType, TZ> &&
			std::is_nothrow_constructible_v<TType, TW>) :
			x(static_cast<TType>(p_xy.x)),
			y(static_cast<TType>(p_xy.y)),
			z(static_cast<TType>(p_z)),
			w(static_cast<TType>(p_w))
		{
		}

		static const IVector4<TType> Zero;
		static const IVector4<TType> One;

		[[nodiscard]] std::string toString() const
		{
			std::ostringstream outputStream;
			outputStream << *this;
			return outputStream.str();
		}

		friend std::ostream& operator<<(std::ostream& p_outputStream, const IVector4<TType>& p_value)
		{
			p_outputStream << '(' << p_value.x << ", " << p_value.y << ", " << p_value.z << ", " << p_value.w << ')';
			return p_outputStream;
		}

		[[nodiscard]] constexpr bool operator==(const IVector4<TType>& p_other) const noexcept
		{
			return spk::ApproxValue(x) == p_other.x && spk::ApproxValue(y) == p_other.y && spk::ApproxValue(z) == p_other.z &&
				   spk::ApproxValue(w) == p_other.w;
		}

		[[nodiscard]] constexpr bool operator!=(const IVector4<TType>& p_other) const noexcept
		{
			return !(*this == p_other);
		}

		[[nodiscard]] constexpr IVector4<TType> operator+(const IVector4<TType>& p_other) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				if (!std::is_constant_evaluated())
				{
					return _fromSimd(_mm_add_ps(_simdValue(), p_other._simdValue()));
				}
			}
#endif
			return IVector4<TType>(x + p_other.x, y + p_other.y, z + p_other.z, w + p_other.w);
		}

		[[nodiscard]] constexpr IVector4<TType> operator-(const IVector4<TType>& p_other) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				if (!std::is_constant_evaluated())
				{
					return _fromSimd(_mm_sub_ps(_simdValue(), p_other._simdValue()));
				}
			}
#endif
			return IVector4<TType>(x - p_other.x, y - p_other.y, z - p_other.z, w - p_other.w);
		}

		[[nodiscard]] constexpr IVector4<TType> operator*(const IVector4<TType>& p_other) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				if (!std::is_constant_evaluated())
				{
					return _fromSimd(_mm_mul_ps(_simdValue(), p_other._simdValue()));
				}
			}
#endif
			return IVector4<TType>(x * p_other.x, y * p_other.y, z * p_other.z, w * p_other.w);
		}

		[[nodiscard]] IVector4<TType> operator/(const IVector4<TType>& p_other) const
		{
			if (spk::ApproxValue(p_other.x) == 0 || spk::ApproxValue(p_other.y) == 0 || spk::ApproxValue(p_other.z) == 0 ||
				spk::ApproxValue(p_other.w) == 0)
			{
				throw std::invalid_argument("spk::IVector4: division by zero vector component");
			}

#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				return _fromSimd(_mm_div_ps(_simdValue(), p_other._simdValue()));
			}
#endif
			return IVector4<TType>(x / p_other.x, y / p_other.y, z / p_other.z, w / p_other.w);
		}

		[[nodiscard]] constexpr IVector4<TType> operator+(const TType& p_scalar) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				if (!std::is_constant_evaluated())
				{
					return _fromSimd(_mm_add_ps(_simdValue(), _mm_set1_ps(p_scalar)));
				}
			}
#endif
			return IVector4<TType>(x + p_scalar, y + p_scalar, z + p_scalar, w + p_scalar);
		}

		[[nodiscard]] constexpr IVector4<TType> operator-(const TType& p_scalar) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				if (!std::is_constant_evaluated())
				{
					return _fromSimd(_mm_sub_ps(_simdValue(), _mm_set1_ps(p_scalar)));
				}
			}
#endif
			return IVector4<TType>(x - p_scalar, y - p_scalar, z - p_scalar, w - p_scalar);
		}

		[[nodiscard]] constexpr IVector4<TType> operator*(const TType& p_scalar) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				if (!std::is_constant_evaluated())
				{
					return _fromSimd(_mm_mul_ps(_simdValue(), _mm_set1_ps(p_scalar)));
				}
			}
#endif
			return IVector4<TType>(x * p_scalar, y * p_scalar, z * p_scalar, w * p_scalar);
		}

		[[nodiscard]] IVector4<TType> operator/(const TType& p_scalar) const
		{
			if (spk::ApproxValue(p_scalar) == 0)
			{
				throw std::invalid_argument("spk::IVector4: division by zero scalar");
			}

#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				return _fromSimd(_mm_div_ps(_simdValue(), _mm_set1_ps(p_scalar)));
			}
#endif
			return IVector4<TType>(x / p_scalar, y / p_scalar, z / p_scalar, w / p_scalar);
		}

		friend constexpr IVector4<TType> operator+(const TType& p_scalar, const IVector4<TType>& p_vector) noexcept
		{
			return p_vector + p_scalar;
		}

		friend constexpr IVector4<TType> operator-(const TType& p_scalar, const IVector4<TType>& p_vector) noexcept
		{
			return IVector4<TType>(p_scalar - p_vector.x, p_scalar - p_vector.y, p_scalar - p_vector.z, p_scalar - p_vector.w);
		}

		friend constexpr IVector4<TType> operator*(const TType& p_scalar, const IVector4<TType>& p_vector) noexcept
		{
			return p_vector * p_scalar;
		}

		friend IVector4<TType> operator/(const TType& p_scalar, const IVector4<TType>& p_vector)
		{
			if (spk::ApproxValue(p_vector.x) == 0 || spk::ApproxValue(p_vector.y) == 0 || spk::ApproxValue(p_vector.z) == 0 ||
				spk::ApproxValue(p_vector.w) == 0)
			{
				throw std::invalid_argument("spk::IVector4: division by zero vector component");
			}

			return IVector4<TType>(p_scalar / p_vector.x, p_scalar / p_vector.y, p_scalar / p_vector.z, p_scalar / p_vector.w);
		}

		[[nodiscard]] constexpr IVector4<TType> inverse() const noexcept
		{
			return IVector4<TType>(-x, -y, -z, -w);
		}

		[[nodiscard]] constexpr IVector4<TType> operator-() const noexcept
		{
			return inverse();
		}

		constexpr IVector4<TType>& operator+=(const IVector4<TType>& p_other) noexcept
		{
			*this = *this + p_other;
			return *this;
		}

		constexpr IVector4<TType>& operator-=(const IVector4<TType>& p_other) noexcept
		{
			*this = *this - p_other;
			return *this;
		}

		constexpr IVector4<TType>& operator*=(const IVector4<TType>& p_other) noexcept
		{
			*this = *this * p_other;
			return *this;
		}

		IVector4<TType>& operator/=(const IVector4<TType>& p_other)
		{
			*this = *this / p_other;
			return *this;
		}

		constexpr IVector4<TType>& operator+=(const TType& p_scalar) noexcept
		{
			*this = *this + p_scalar;
			return *this;
		}

		constexpr IVector4<TType>& operator-=(const TType& p_scalar) noexcept
		{
			*this = *this - p_scalar;
			return *this;
		}

		constexpr IVector4<TType>& operator*=(const TType& p_scalar) noexcept
		{
			*this = *this * p_scalar;
			return *this;
		}

		IVector4<TType>& operator/=(const TType& p_scalar)
		{
			*this = *this / p_scalar;
			return *this;
		}

		[[nodiscard]] constexpr bool isZero() const noexcept
		{
			return spk::ApproxValue(x) == 0 && spk::ApproxValue(y) == 0 && spk::ApproxValue(z) == 0 && spk::ApproxValue(w) == 0;
		}

		[[nodiscard]] float squaredLength() const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				const __m128 multiplied = _mm_mul_ps(_simdValue(), _simdValue());
				alignas(16) float values[4];
				_mm_store_ps(values, multiplied);
				return values[0] + values[1] + values[2] + values[3];
			}
#endif
			const float valueX = static_cast<float>(x);
			const float valueY = static_cast<float>(y);
			const float valueZ = static_cast<float>(z);
			const float valueW = static_cast<float>(w);
			return (valueX * valueX) + (valueY * valueY) + (valueZ * valueZ) + (valueW * valueW);
		}

		[[nodiscard]] float length() const noexcept
		{
			return static_cast<float>(std::sqrt(static_cast<double>(squaredLength())));
		}

		[[nodiscard]] IVector4<float> normalized() const
		{
			const float squaredVectorLength = squaredLength();

			if (spk::ApproxValue(squaredVectorLength) == 0)
			{
				throw std::runtime_error("spk::IVector4: cannot normalize a zero-length vector");
			}

			const float inverseLength = 1.0f / static_cast<float>(std::sqrt(static_cast<double>(squaredVectorLength)));

			return IVector4<float>(
				static_cast<float>(x) * inverseLength,
				static_cast<float>(y) * inverseLength,
				static_cast<float>(z) * inverseLength,
				static_cast<float>(w) * inverseLength);
		}

		[[nodiscard]] float squaredDistance(const IVector4<TType>& p_other) const noexcept
		{
			return (*this - p_other).squaredLength();
		}

		[[nodiscard]] float distance(const IVector4<TType>& p_other) const noexcept
		{
			return (*this - p_other).length();
		}

		[[nodiscard]] float dot(const IVector4<TType>& p_other) const noexcept
		{
#if SPK_VECTOR4_HAS_SSE
			if constexpr (std::same_as<TType, float>)
			{
				const __m128 multiplied = _mm_mul_ps(_simdValue(), p_other._simdValue());
				alignas(16) float values[4];
				_mm_store_ps(values, multiplied);
				return values[0] + values[1] + values[2] + values[3];
			}
#endif
			return (static_cast<float>(x) * static_cast<float>(p_other.x)) + (static_cast<float>(y) * static_cast<float>(p_other.y)) +
				   (static_cast<float>(z) * static_cast<float>(p_other.z)) + (static_cast<float>(w) * static_cast<float>(p_other.w));
		}

		[[nodiscard]] constexpr IVector2<TType> xy() const noexcept
		{
			return IVector2<TType>(x, y);
		}

		[[nodiscard]] constexpr IVector3<TType> xyz() const noexcept
		{
			return IVector3<TType>(x, y, z);
		}

		[[nodiscard]] static IVector4<TType> lerp(const IVector4<TType>& p_from, const IVector4<TType>& p_to, const float p_alpha) noexcept
		{
			const float fromX = static_cast<float>(p_from.x);
			const float fromY = static_cast<float>(p_from.y);
			const float fromZ = static_cast<float>(p_from.z);
			const float fromW = static_cast<float>(p_from.w);

			return IVector4<TType>(
				static_cast<TType>(fromX + ((static_cast<float>(p_to.x) - fromX) * p_alpha)),
				static_cast<TType>(fromY + ((static_cast<float>(p_to.y) - fromY) * p_alpha)),
				static_cast<TType>(fromZ + ((static_cast<float>(p_to.z) - fromZ) * p_alpha)),
				static_cast<TType>(fromW + ((static_cast<float>(p_to.w) - fromW) * p_alpha)));
		}

		[[nodiscard]] static constexpr IVector4<TType> min(const IVector4<TType>& p_left, const IVector4<TType>& p_right) noexcept
		{
			return IVector4<TType>(
				std::min(p_left.x, p_right.x), std::min(p_left.y, p_right.y), std::min(p_left.z, p_right.z), std::min(p_left.w, p_right.w));
		}

		template <typename... TOtherTypes>
			requires(sizeof...(TOtherTypes) > 0) && (std::same_as<TOtherTypes, IVector4<TType>> && ...)
		[[nodiscard]] static constexpr IVector4<TType> min(
			const IVector4<TType>& p_first, const IVector4<TType>& p_second, const TOtherTypes&... p_remainingValues) noexcept
		{
			return min(min(p_first, p_second), p_remainingValues...);
		}

		[[nodiscard]] static IVector4<TType> min(std::initializer_list<IVector4<TType>> p_values)
		{
			if (p_values.size() == 0)
			{
				throw std::invalid_argument("spk::IVector4::min requires at least one value");
			}

			IVector4<TType> result = *p_values.begin();
			for (auto iterator = std::next(p_values.begin()); iterator != p_values.end(); ++iterator)
			{
				result = min(result, *iterator);
			}
			return result;
		}

		[[nodiscard]] static constexpr IVector4<TType> max(const IVector4<TType>& p_left, const IVector4<TType>& p_right) noexcept
		{
			return IVector4<TType>(
				std::max(p_left.x, p_right.x), std::max(p_left.y, p_right.y), std::max(p_left.z, p_right.z), std::max(p_left.w, p_right.w));
		}

		template <typename... TOtherTypes>
			requires(sizeof...(TOtherTypes) > 0) && (std::same_as<TOtherTypes, IVector4<TType>> && ...)
		[[nodiscard]] static constexpr IVector4<TType> max(
			const IVector4<TType>& p_first, const IVector4<TType>& p_second, const TOtherTypes&... p_remainingValues) noexcept
		{
			return max(max(p_first, p_second), p_remainingValues...);
		}

		[[nodiscard]] static IVector4<TType> max(std::initializer_list<IVector4<TType>> p_values)
		{
			if (p_values.size() == 0)
			{
				throw std::invalid_argument("spk::IVector4::max requires at least one value");
			}

			IVector4<TType> result = *p_values.begin();
			for (auto iterator = std::next(p_values.begin()); iterator != p_values.end(); ++iterator)
			{
				result = max(result, *iterator);
			}
			return result;
		}

		[[nodiscard]] static constexpr IVector4<TType> clamp(
			const IVector4<TType>& p_value, const IVector4<TType>& p_boundMinimum, const IVector4<TType>& p_boundMaximum) noexcept
		{
			const IVector4<TType> low = min(p_boundMinimum, p_boundMaximum);
			const IVector4<TType> high = max(p_boundMinimum, p_boundMaximum);

			return IVector4<TType>(
				std::clamp(p_value.x, low.x, high.x),
				std::clamp(p_value.y, low.y, high.y),
				std::clamp(p_value.z, low.z, high.z),
				std::clamp(p_value.w, low.w, high.w));
		}

		[[nodiscard]] constexpr IVector4<TType> clamped(const IVector4<TType>& p_boundMinimum, const IVector4<TType>& p_boundMaximum) const noexcept
		{
			return clamp(*this, p_boundMinimum, p_boundMaximum);
		}

		[[nodiscard]] static constexpr bool isBetween(
			const IVector4<TType>& p_value, const IVector4<TType>& p_boundA, const IVector4<TType>& p_boundB) noexcept
		{
			const IVector4<TType> low = min(p_boundA, p_boundB);
			const IVector4<TType> high = max(p_boundA, p_boundB);

			return (p_value.x >= low.x && p_value.x <= high.x) && (p_value.y >= low.y && p_value.y <= high.y) &&
				   (p_value.z >= low.z && p_value.z <= high.z) && (p_value.w >= low.w && p_value.w <= high.w);
		}

		template <typename TOther>
			requires std::constructible_from<TOther, TType>
		explicit constexpr operator IVector4<TOther>() const noexcept(std::is_nothrow_constructible_v<TOther, TType>)
		{
			return IVector4<TOther>(static_cast<TOther>(x), static_cast<TOther>(y), static_cast<TOther>(z), static_cast<TOther>(w));
		}
	};

	template <spk::arithmetic_value TType>
	inline const IVector4<TType> IVector4<TType>::Zero{};

	template <spk::arithmetic_value TType>
	inline const IVector4<TType> IVector4<TType>::One{static_cast<TType>(1), static_cast<TType>(1), static_cast<TType>(1), static_cast<TType>(1)};

	using Vector4 = IVector4<float>;
	using Vector4Int = IVector4<std::int32_t>;
	using Vector4UInt = IVector4<std::uint32_t>;
}

#undef SPK_VECTOR4_HAS_SSE
