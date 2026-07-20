#pragma once

#include <concepts>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace spk
{
	/**
	 * A dynamically growing, FIFO queue whose individual operations are synchronized.
	 *
	 * tryPop() atomically checks for and removes the oldest value without waiting for
	 * a future producer. empty() and size() are snapshots only. Concurrent pushes are
	 * ordered by their acquisition of the queue lock. drain() atomically detaches the
	 * current batch: pushes before its swap are returned, while later pushes remain in
	 * the queue. The queue's lifetime must be externally synchronized with its users.
	 */
	template <typename TType>
	class ThreadSafeQueue
	{
	public:
		using ValueType = TType;
		using SizeType = typename std::deque<TType>::size_type;

		ThreadSafeQueue() = default;

		ThreadSafeQueue(const ThreadSafeQueue &) = delete;
		ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

		ThreadSafeQueue(ThreadSafeQueue &&) = delete;
		ThreadSafeQueue &operator=(ThreadSafeQueue &&) = delete;

		void push(TType p_value)
		{
			std::lock_guard lock(_mutex);
			_values.push_back(std::move(p_value));
		}

		// Construction occurs while the queue is locked. Prepare expensive values before calling push().
		template <typename... TArguments>
		void emplace(TArguments &&...p_arguments)
		{
			std::lock_guard lock(_mutex);
			_values.emplace_back(std::forward<TArguments>(p_arguments)...);
		}

		/**
		 * Atomically retrieves and removes the oldest value, or returns std::nullopt.
		 * If moving TType throws, the element remains queued but may be moved from.
		 */
		[[nodiscard]] std::optional<TType> tryPop()
			requires std::move_constructible<TType>
		{
			std::lock_guard lock(_mutex);

			if (_values.empty())
			{
				return std::nullopt;
			}

			std::optional<TType> result(std::in_place, std::move(_values.front()));
			_values.pop_front();
			return result;
		}

		[[nodiscard]] std::deque<TType> drain()
		{
			std::deque<TType> result;

			{
				std::lock_guard lock(_mutex);
				result.swap(_values);
			}

			return result;
		}

		void clear()
		{
			std::deque<TType> values;

			{
				std::lock_guard lock(_mutex);
				values.swap(_values);
			}
		}

		[[nodiscard]] bool empty() const
		{
			std::lock_guard lock(_mutex);
			return _values.empty();
		}

		[[nodiscard]] SizeType size() const
		{
			std::lock_guard lock(_mutex);
			return _values.size();
		}

	private:
		mutable std::mutex _mutex;
		std::deque<TType> _values;
	};
}
