#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

namespace spk
{
	template <typename TEnumType>
	concept enum_type =
		std::is_enum_v<TEnumType>;

	template <typename TType>
	concept unsigned_storage =
		std::is_unsigned_v<TType> &&
		(sizeof(TType) == 1 || sizeof(TType) == 2 || sizeof(TType) == 4);

	template <typename TType>
	concept hashable =
		requires(const TType &p_value) {
			{ std::hash<TType>{}(p_value) } -> std::convertible_to<std::size_t>;
		};

	template <typename TType>
	concept arithmetic_value =
		std::is_arithmetic_v<TType>;

	template <typename TLeft, typename TRight = TLeft>
	concept equality_comparable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			{ p_left == p_right } -> std::convertible_to<bool>;
			{ p_left != p_right } -> std::convertible_to<bool>;
		};

	template <typename TType>
	concept comparison_compatible =
		equality_comparable_with<TType, TType>;

	template <typename TLeft, typename TRight = TLeft>
	concept addable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			p_left + p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept add_assignable_with =
		requires(TLeft &p_left, const TRight &p_right) {
			p_left += p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept subtractable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			p_left - p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept subtract_assignable_with =
		requires(TLeft &p_left, const TRight &p_right) {
			p_left -= p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept multiplicable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			p_left * p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept multiply_assignable_with =
		requires(TLeft &p_left, const TRight &p_right) {
			p_left *= p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept dividable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			p_left / p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept divide_assignable_with =
		requires(TLeft &p_left, const TRight &p_right) {
			p_left /= p_right;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept less_than_comparable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			{ p_left < p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept less_equal_comparable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			{ p_left <= p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept greater_than_comparable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
			{ p_left > p_right } -> std::convertible_to<bool>;
		};

	template <typename TLeft, typename TRight = TLeft>
	concept greater_equal_comparable_with =
		requires(const TLeft &p_left, const TRight &p_right) {
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
		requires(TType &p_value) {
			++p_value;
		};

	template <typename TType>
	concept post_incrementable =
		requires(TType &p_value) {
			p_value++;
		};

	template <typename TType>
	concept incrementable =
		pre_incrementable<TType> && post_incrementable<TType>;

	template <typename TType>
	concept pre_decrementable =
		requires(TType &p_value) {
			--p_value;
		};

	template <typename TType>
	concept post_decrementable =
		requires(TType &p_value) {
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

	template <typename T>
	concept native_boolean = std::same_as<std::remove_cvref_t<T>, bool>;

	template <typename T>
	concept native_integer =
		std::integral<std::remove_cvref_t<T>> &&
		!std::same_as<std::remove_cvref_t<T>, bool> &&
		!std::same_as<std::remove_cvref_t<T>, wchar_t> &&
		!std::same_as<std::remove_cvref_t<T>, char> &&
		!std::same_as<std::remove_cvref_t<T>, char8_t> &&
		!std::same_as<std::remove_cvref_t<T>, char16_t> &&
		!std::same_as<std::remove_cvref_t<T>, char32_t>;

	template <typename T>
	concept native_floating = std::floating_point<std::remove_cvref_t<T>>;

	template <typename T>
	concept native_string =
		std::same_as<std::remove_cvref_t<T>, std::string> ||
		std::same_as<std::remove_cvref_t<T>, std::string_view>;

	template <typename T>
	concept native_null = std::same_as<std::remove_cvref_t<T>, std::nullptr_t>;

	template <typename T>
	concept native_value = native_null<T> || native_boolean<T> || native_integer<T> || native_floating<T> || native_string<T>;
}
