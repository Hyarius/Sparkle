#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TValue>
	class WeightedPool
	{
	public:
		struct Entry
		{
			TValue value;
			double weight = 1.0;
		};

	private:
		std::vector<Entry> _entries;
		std::vector<double> _cumulativeWeights;
		double _totalWeight = 0.0;

		[[nodiscard]] double _validateWeight(double p_weight) const
		{
			if (!std::isfinite(p_weight) || p_weight <= 0.0)
			{
				throw std::invalid_argument("weighted-pool entries need a finite positive weight");
			}
			const double newTotalWeight = _totalWeight + p_weight;
			if (!std::isfinite(newTotalWeight) || newTotalWeight == _totalWeight)
			{
				throw std::overflow_error("weighted-pool total weight cannot represent the new entry");
			}
			return newTotalWeight;
		}

		template <typename TFactory>
		TValue &_insert(double p_weight, TFactory &&p_factory)
		{
			const double newTotalWeight = _validateWeight(p_weight);
			_entries.emplace_back(p_factory(), p_weight);
			try
			{
				_cumulativeWeights.push_back(newTotalWeight);
			} catch (...)
			{
				_entries.pop_back();
				throw;
			}
			_totalWeight = newTotalWeight;
			return _entries.back().value;
		}

		void _validateTarget(double p_target) const
		{
			if (_entries.empty())
			{
				throw std::logic_error("cannot pick from an empty weighted pool");
			}
			if (!std::isfinite(p_target) || p_target < 0.0 || p_target >= _totalWeight)
			{
				throw std::out_of_range("weighted-pool target must be in [0, totalWeight)");
			}
		}

	public:
		WeightedPool() = default;

		WeightedPool(std::initializer_list<Entry> p_entries)
			requires std::copy_constructible<TValue>
		{
			reserve(p_entries.size());
			for (const Entry &entry : p_entries)
			{
				add(entry.value, entry.weight);
			}
		}

		void reserve(std::size_t p_count)
		{
			_entries.reserve(p_count);
			_cumulativeWeights.reserve(p_count);
		}

		void add(TValue p_value, double p_weight = 1.0)
		{
			(void)_insert(p_weight, [&]() -> TValue {
				return std::move(p_value);
			});
		}

		template <typename... TArguments>
		TValue &emplace(double p_weight, TArguments &&...p_arguments)
		{
			return _insert(p_weight, [&]() -> TValue {
				return TValue(std::forward<TArguments>(p_arguments)...);
			});
		}

		void clear() noexcept
		{
			_entries.clear();
			_cumulativeWeights.clear();
			_totalWeight = 0.0;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _entries.empty();
		}
		[[nodiscard]] std::size_t size() const noexcept
		{
			return _entries.size();
		}
		[[nodiscard]] double totalWeight() const noexcept
		{
			return _totalWeight;
		}
		[[nodiscard]] const Entry &front() const
		{
			return _entries.front();
		}
		[[nodiscard]] const Entry &back() const
		{
			return _entries.back();
		}
		[[nodiscard]] auto begin() const noexcept
		{
			return _entries.begin();
		}
		[[nodiscard]] auto end() const noexcept
		{
			return _entries.end();
		}

		// Selection uses half-open intervals. A target exactly on an entry boundary
		// belongs to the following entry.
		[[nodiscard]] const TValue &pick(double p_unitRoll) const
		{
			if (!std::isfinite(p_unitRoll) || p_unitRoll < 0.0 || p_unitRoll >= 1.0)
			{
				throw std::out_of_range("weighted-pool unit roll must be in [0, 1)");
			}
			double target = p_unitRoll * _totalWeight;
			if (target >= _totalWeight)
			{
				target = std::nextafter(_totalWeight, 0.0);
			}
			return pickTarget(target);
		}

		[[nodiscard]] const TValue &pickTarget(double p_target) const
		{
			_validateTarget(p_target);
			const auto iterator = std::upper_bound(_cumulativeWeights.begin(), _cumulativeWeights.end(), p_target);
			return _entries[static_cast<std::size_t>(std::distance(_cumulativeWeights.begin(), iterator))].value;
		}
	};
}
