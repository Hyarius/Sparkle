#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TType>
	class ThreadSafeDeque
	{
	private:
		mutable std::mutex _mutex;
		std::deque<TType> _values;

	public:
		ThreadSafeDeque() = default;

		ThreadSafeDeque(const ThreadSafeDeque&) = delete;
		ThreadSafeDeque& operator=(const ThreadSafeDeque&) = delete;

		ThreadSafeDeque(ThreadSafeDeque&&) = delete;
		ThreadSafeDeque& operator=(ThreadSafeDeque&&) = delete;

		void pushBack(TType p_value)
		{
			std::scoped_lock lock(_mutex);
			_values.push_back(std::move(p_value));
		}

		template <typename... TArguments>
		void emplaceBack(TArguments&&... p_arguments)
		{
			std::scoped_lock lock(_mutex);
			_values.emplace_back(std::forward<TArguments>(p_arguments)...);
		}

		[[nodiscard]] bool popFront(TType& p_output)
		{
			std::scoped_lock lock(_mutex);

			if (_values.empty() == true)
			{
				return false;
			}

			p_output = std::move(_values.front());
			_values.pop_front();
			return true;
		}

		[[nodiscard]] std::vector<TType> drain()
		{
			std::deque<TType> values;

			{
				std::scoped_lock lock(_mutex);
				values.swap(_values);
			}

			std::vector<TType> result;
			result.reserve(values.size());

			for (TType& value : values)
			{
				result.push_back(std::move(value));
			}

			return result;
		}

		void clear()
		{
			std::scoped_lock lock(_mutex);
			_values.clear();
		}

		[[nodiscard]] bool empty() const
		{
			std::scoped_lock lock(_mutex);
			return _values.empty();
		}

		[[nodiscard]] size_t size() const
		{
			std::scoped_lock lock(_mutex);
			return _values.size();
		}
	};
}
