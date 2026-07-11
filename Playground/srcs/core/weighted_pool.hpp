#pragma once

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pg
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
		double _totalWeight = 0.0;

	public:
		WeightedPool() = default;

		WeightedPool(std::initializer_list<Entry> p_entries)
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
		}

		void add(TValue p_value, double p_weight = 1.0)
		{
			if (!std::isfinite(p_weight) || p_weight <= 0.0)
			{
				throw std::invalid_argument("weighted-pool entries need a finite positive weight");
			}
			if (!std::isfinite(_totalWeight + p_weight))
			{
				throw std::overflow_error("weighted-pool total weight must be finite");
			}
			_entries.push_back({.value = std::move(p_value), .weight = p_weight});
			_totalWeight += p_weight;
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

		// Selects from a roll in [0, 1). Callers remain responsible for how that
		// roll is seeded and generated, so adopting the pool does not couple their
		// deterministic streams.
		[[nodiscard]] const TValue &pick(double p_unitRoll) const
		{
			return pickTarget(p_unitRoll * _totalWeight);
		}

		// Retains selection loops that historically treated an exact weight
		// boundary as belonging to the earlier entry.
		[[nodiscard]] const TValue &pickInclusive(double p_unitRoll) const
		{
			return pickTargetInclusive(p_unitRoll * _totalWeight);
		}

		// Selects from a roll in [0, totalWeight). This overload lets callers keep
		// an existing distribution whose bounds already use the pool total.
		[[nodiscard]] const TValue &pickTarget(double p_target) const
		{
			if (_entries.empty())
			{
				throw std::logic_error("cannot pick from an empty weighted pool");
			}
			for (const Entry &entry : _entries)
			{
				if (p_target < entry.weight)
				{
					return entry.value;
				}
				p_target -= entry.weight;
			}
			return _entries.back().value;
		}

		[[nodiscard]] const TValue &pickTargetInclusive(double p_target) const
		{
			if (_entries.empty())
			{
				throw std::logic_error("cannot pick from an empty weighted pool");
			}
			for (const Entry &entry : _entries)
			{
				if (p_target <= entry.weight)
				{
					return entry.value;
				}
				p_target -= entry.weight;
			}
			return _entries.back().value;
		}
	};
}
