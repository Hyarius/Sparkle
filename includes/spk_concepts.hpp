#pragma once

#include <concepts>

namespace spk
{
	template <typename TLeft, typename TRight = TLeft>
	concept equality_comparable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			{ p_left == p_right } -> std::convertible_to<bool>;
			{ p_left != p_right } -> std::convertible_to<bool>;
		};

	template <typename TType>
	concept comparison_compatible =
		equality_comparable_with<TType, TType>;

	template <typename TLeft, typename TRight = TLeft>
	concept addable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			p_left + p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept add_assignable_with =
		requires(TLeft& p_left, const TRight& p_right)
		{
			p_left += p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept subtractable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			p_left - p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept subtract_assignable_with =
		requires(TLeft& p_left, const TRight& p_right)
		{
			p_left -= p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept multiplicable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			p_left * p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept multiply_assignable_with =
		requires(TLeft& p_left, const TRight& p_right)
		{
			p_left *= p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept dividable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			p_left / p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept divide_assignable_with =
		requires(TLeft& p_left, const TRight& p_right)
		{
			p_left /= p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept less_than_comparable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			{ p_left < p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept less_equal_comparable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			{ p_left <= p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept greater_than_comparable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			{ p_left > p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept greater_equal_comparable_with =
		requires(const TLeft& p_left, const TRight& p_right)
		{
			{ p_left >= p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept order_comparable_with =
		less_than_comparable_with<TLeft, TRight> &&
		less_equal_comparable_with<TLeft, TRight> &&
		greater_than_comparable_with<TLeft, TRight> &&
		greater_equal_comparable_with<TLeft, TRight>;

	template <typename TType>
	concept pre_incrementable =
		requires(TType& p_value)
		{
			++p_value;
		};

	template <typename TType>
	concept post_incrementable =
		requires(TType& p_value)
		{
			p_value++;
		};

	template <typename TType>
	concept incrementable =
		pre_incrementable<TType> && post_incrementable<TType>;

	template <typename TType>
	concept pre_decrementable =
		requires(TType& p_value)
		{
			--p_value;
		};

	template <typename TType>
	concept post_decrementable =
		requires(TType& p_value)
		{
			p_value--;
		};

	template <typename TType>
	concept decrementable =
		pre_decrementable<TType> && post_decrementable<TType>;

	template <typename TType>
	concept arithmetic_like =
		addable_with<TType> &&
		add_assignable_with<TType> &&
		subtractable_with<TType> &&
		subtract_assignable_with<TType> &&
		multiplicable_with<TType> &&
		multiply_assignable_with<TType> &&
		dividable_with<TType> &&
		divide_assignable_with<TType>;
}