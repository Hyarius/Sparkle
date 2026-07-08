#pragma once

#include <concepts>
#include <type_traits>

#include "type/spk_concepts.hpp"

namespace spk::JSON
{
	class Value;
	using Object = Value;

	template <typename T>
	concept json_writable =
		requires(const std::remove_cvref_t<T> &p_value) {
			{ toJSON(p_value) } -> std::convertible_to<Value>;
		};

	template <typename T>
	concept json_readable =
		std::default_initializable<std::remove_cvref_t<T>> &&
		requires(std::remove_cvref_t<T> &p_value, const Value &p_object) {
			{ fromJSON(p_object, p_value) } -> std::same_as<void>;
		};
}
