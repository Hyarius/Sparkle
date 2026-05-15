#pragma once

#include <concepts>
#include <utility>

#include "spk_contract_provider.hpp"
#include "spk_concepts.hpp"

namespace spk
{
	template <typename TType>
		requires std::move_constructible<TType> &&
				 std::default_initializable<TType> &&
				 spk::comparison_compatible<TType>
	class ObservableValue : public ContractProvider<const TType&>
	{
	private:
		TType _value = TType();

		using ContractProvider<const TType&>::trigger;

		ObservableValue& _assign(const TType& p_value)
		{
			if (_value != p_value)
			{
				_value = p_value;
				trigger(_value);
			}

			return *this;
		}

		ObservableValue& _assign(TType&& p_value)
		{
			if (_value != p_value)
			{
				_value = std::move(p_value);
				trigger(_value);
			}

			return *this;
		}

	public:
		ObservableValue() = default;

		ObservableValue(const TType& p_value) :
			_value(p_value)
		{
		}

		ObservableValue(TType&& p_value) :
			_value(std::move(p_value))
		{
		}

		operator const TType&() const
		{
			return _value;
		}

		const TType& value() const
		{
			return _value;
		}

		template <typename TOther>
			requires std::assignable_from<TType&, const TOther&>
		ObservableValue& set(const TOther& p_value)
		{
			TType newValue = _value;
			newValue = p_value;
			return _assign(std::move(newValue));
		}

		template <typename TOther>
			requires std::assignable_from<TType&, const TOther&>
		ObservableValue& operator=(const TOther& p_value)
		{
			TType newValue = _value;
			newValue = p_value;
			return _assign(std::move(newValue));
		}

		template <typename TOther>
			requires spk::addable_with<TType, TOther>
		auto operator+(const TOther& p_value) const -> decltype(_value + p_value)
		{
			return _value + p_value;
		}

		template <typename TOther>
			requires spk::subtractable_with<TType, TOther>
		auto operator-(const TOther& p_value) const -> decltype(_value - p_value)
		{
			return _value - p_value;
		}

		template <typename TOther>
			requires spk::multiplicable_with<TType, TOther>
		auto operator*(const TOther& p_value) const -> decltype(_value * p_value)
		{
			return _value * p_value;
		}

		template <typename TOther>
			requires spk::dividable_with<TType, TOther>
		auto operator/(const TOther& p_value) const -> decltype(_value / p_value)
		{
			return _value / p_value;
		}

		template <typename TOther>
			requires spk::add_assignable_with<TType, TOther>
		ObservableValue& operator+=(const TOther& p_value)
		{
			TType newValue = _value;
			newValue += p_value;
			return _assign(std::move(newValue));
		}

		template <typename TOther>
			requires spk::subtract_assignable_with<TType, TOther>
		ObservableValue& operator-=(const TOther& p_value)
		{
			TType newValue = _value;
			newValue -= p_value;
			return _assign(std::move(newValue));
		}

		template <typename TOther>
			requires spk::multiply_assignable_with<TType, TOther>
		ObservableValue& operator*=(const TOther& p_value)
		{
			TType newValue = _value;
			newValue *= p_value;
			return _assign(std::move(newValue));
		}

		template <typename TOther>
			requires spk::divide_assignable_with<TType, TOther>
		ObservableValue& operator/=(const TOther& p_value)
		{
			TType newValue = _value;
			newValue /= p_value;
			return _assign(std::move(newValue));
		}

		template <typename TOther>
			requires spk::equality_comparable_with<TType, TOther>
		bool operator==(const TOther& p_value) const
		{
			return _value == p_value;
		}

		template <typename TOther>
			requires spk::equality_comparable_with<TType, TOther>
		bool operator!=(const TOther& p_value) const
		{
			return _value != p_value;
		}

		template <typename TOther>
			requires spk::less_than_comparable_with<TType, TOther>
		bool operator<(const TOther& p_value) const
		{
			return _value < p_value;
		}

		template <typename TOther>
			requires spk::less_equal_comparable_with<TType, TOther>
		bool operator<=(const TOther& p_value) const
		{
			return _value <= p_value;
		}

		template <typename TOther>
			requires spk::greater_than_comparable_with<TType, TOther>
		bool operator>(const TOther& p_value) const
		{
			return _value > p_value;
		}

		template <typename TOther>
			requires spk::greater_equal_comparable_with<TType, TOther>
		bool operator>=(const TOther& p_value) const
		{
			return _value >= p_value;
		}

		ObservableValue& operator++()
			requires spk::pre_incrementable<TType>
		{
			TType newValue = _value;
			++newValue;
			return _assign(std::move(newValue));
		}

		ObservableValue& operator--()
			requires spk::pre_decrementable<TType>
		{
			TType newValue = _value;
			--newValue;
			return _assign(std::move(newValue));
		}

		TType operator++(int)
			requires spk::post_incrementable<TType>
		{
			TType result = _value;
			TType newValue = _value;
			newValue++;
			_assign(std::move(newValue));
			return result;
		}

		TType operator--(int)
			requires spk::post_decrementable<TType>
		{
			TType result = _value;
			TType newValue = _value;
			newValue--;
			_assign(std::move(newValue));
			return result;
		}
	};
}