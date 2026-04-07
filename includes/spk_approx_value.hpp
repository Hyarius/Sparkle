#pragma once

#include <cmath>
#include <concepts>
#include <type_traits>

#include "spk_constants.hpp"

namespace spk
{
	template <typename TType>
	concept approx_value_type =
		std::is_arithmetic_v<TType>;

	template <approx_value_type TType, bool IsFloatingPoint = std::is_floating_point_v<TType>>
	struct ApproxValue;

	template <approx_value_type TType>
	struct ApproxValue<TType, false>
	{
		using value_type = TType;

		TType value{};

		constexpr ApproxValue() = default;

		explicit constexpr ApproxValue(const TType& p_value) noexcept :
			value(p_value)
		{
		}
	};

	template <approx_value_type TType>
	struct ApproxValue<TType, true>
	{
		using value_type = TType;

		TType value{};
		TType epsilon{static_cast<TType>(spk::Math::Constants::pointPrecision)};

		constexpr ApproxValue() = default;

		explicit constexpr ApproxValue(const TType& p_value) noexcept :
			value(p_value)
		{
		}

		constexpr ApproxValue(const TType& p_value, const TType& p_epsilon) noexcept :
			value(p_value),
			epsilon(p_epsilon)
		{
		}
	};

	template <typename TType>
	ApproxValue(TType) -> ApproxValue<std::remove_cvref_t<TType>>;

	template <typename TType, typename TOther>
	ApproxValue(TType, TOther) -> ApproxValue<std::remove_cvref_t<TType>>;

	template <typename TLeftHandType, bool IsLeftHandFloatingPoint, typename TRightHandType>
		requires approx_value_type<TLeftHandType> &&
				 approx_value_type<TRightHandType>
	[[nodiscard]] constexpr bool operator==(
		const ApproxValue<TLeftHandType, IsLeftHandFloatingPoint>& p_left,
		const TRightHandType& p_right)
	{
		using CommonType = std::common_type_t<TLeftHandType, TRightHandType>;

		if constexpr (IsLeftHandFloatingPoint)
		{
			const CommonType delta = static_cast<CommonType>(p_left.value) - static_cast<CommonType>(p_right);
			const CommonType epsilon = static_cast<CommonType>(p_left.epsilon);
			return std::abs(delta) <= epsilon;
		}
		else
		{
			if constexpr (std::is_floating_point_v<TRightHandType>)
			{
				const CommonType delta = static_cast<CommonType>(p_left.value) - static_cast<CommonType>(p_right);
				const CommonType epsilon = static_cast<CommonType>(spk::Math::Constants::pointPrecision);
				return std::abs(delta) <= epsilon;
			}
			else
			{
				return static_cast<CommonType>(p_left.value) == static_cast<CommonType>(p_right);
			}
		}
	}

	template <typename TLeftHandType, typename TRightHandType, bool IsRightHandFloatingPoint>
		requires approx_value_type<TLeftHandType> &&
				 approx_value_type<TRightHandType>
	[[nodiscard]] constexpr bool operator==(
		const TLeftHandType& p_left,
		const ApproxValue<TRightHandType, IsRightHandFloatingPoint>& p_right)
	{
		return p_right == p_left;
	}

	template <typename TLeftHandType, bool IsLeftHandFloatingPoint, typename TRightHandType, bool IsRightHandFloatingPoint>
		requires approx_value_type<TLeftHandType> &&
				 approx_value_type<TRightHandType>
	[[nodiscard]] constexpr bool operator==(
		const ApproxValue<TLeftHandType, IsLeftHandFloatingPoint>& p_left,
		const ApproxValue<TRightHandType, IsRightHandFloatingPoint>& p_right)
	{
		using CommonType = std::common_type_t<TLeftHandType, TRightHandType>;

		if constexpr (IsLeftHandFloatingPoint || IsRightHandFloatingPoint)
		{
			const CommonType delta = static_cast<CommonType>(p_left.value) - static_cast<CommonType>(p_right.value);

			CommonType epsilon{};

			if constexpr (IsLeftHandFloatingPoint && IsRightHandFloatingPoint)
			{
				epsilon = std::min(
					static_cast<CommonType>(p_left.epsilon),
					static_cast<CommonType>(p_right.epsilon));
			}
			else if constexpr (IsLeftHandFloatingPoint)
			{
				epsilon = static_cast<CommonType>(p_left.epsilon);
			}
			else
			{
				epsilon = static_cast<CommonType>(p_right.epsilon);
			}

			return std::abs(delta) <= epsilon;
		}
		else
		{
			return static_cast<CommonType>(p_left.value) == static_cast<CommonType>(p_right.value);
		}
	}

	template <typename TLeftHandType, bool IsLeftHandFloatingPoint, typename TRightHandType>
		requires approx_value_type<TLeftHandType> &&
				 approx_value_type<TRightHandType>
	[[nodiscard]] constexpr bool operator!=(
		const ApproxValue<TLeftHandType, IsLeftHandFloatingPoint>& p_left,
		const TRightHandType& p_right)
	{
		return !(p_left == p_right);
	}

	template <typename TLeftHandType, typename TRightHandType, bool IsRightHandFloatingPoint>
		requires approx_value_type<TLeftHandType> &&
				 approx_value_type<TRightHandType>
	[[nodiscard]] constexpr bool operator!=(
		const TLeftHandType& p_left,
		const ApproxValue<TRightHandType, IsRightHandFloatingPoint>& p_right)
	{
		return !(p_left == p_right);
	}

	template <typename TLeftHandType, bool IsLeftHandFloatingPoint, typename TRightHandType, bool IsRightHandFloatingPoint>
		requires approx_value_type<TLeftHandType> &&
				 approx_value_type<TRightHandType>
	[[nodiscard]] constexpr bool operator!=(
		const ApproxValue<TLeftHandType, IsLeftHandFloatingPoint>& p_left,
		const ApproxValue<TRightHandType, IsRightHandFloatingPoint>& p_right)
	{
		return !(p_left == p_right);
	}
}