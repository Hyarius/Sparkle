#pragma once

#include <chrono>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk
{
	// Persistent bounded worker set. Submitted work is drained before destruction;
	// task results and failures are retrieved through Task<TResult>::obtain().
	class WorkerPool
	{
	public:
		static constexpr std::size_t AutomaticWorkerLimit = 8;

		template <typename TResult>
		class Task
		{
			friend class WorkerPool;

		private:
			std::future<TResult> _future;

			explicit Task(std::future<TResult> p_future) :
				_future(std::move(p_future))
			{
			}

		public:
			Task() = default;
			Task(Task &&) noexcept = default;
			Task &operator=(Task &&) noexcept = default;

			Task(const Task &) = delete;
			Task &operator=(const Task &) = delete;

			[[nodiscard]] bool valid() const noexcept
			{
				return _future.valid();
			}

			[[nodiscard]] bool isComplete() const
			{
				return valid() && _future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
			}

			void wait() const
			{
				_future.wait();
			}

			[[nodiscard]] TResult obtain()
				requires(!std::is_void_v<TResult>)
			{
				return _future.get();
			}

			void obtain()
				requires std::is_void_v<TResult>
			{
				_future.get();
			}
		};

	private:
		struct Work
		{
			std::function<void()> function;
		};

		std::deque<Work> _queue;
		std::vector<std::jthread> _workers;
		mutable std::mutex _mutex;
		std::condition_variable_any _workAvailable;
		std::condition_variable _workCompleted;
		std::size_t _unfinishedWorkCount = 0;
		bool _acceptingWork = true;

		void _push(std::function<void()> p_work);
		void _run(std::stop_token p_stopToken);

	public:
		explicit WorkerPool(std::size_t p_workerCount = 0);
		~WorkerPool();

		WorkerPool(const WorkerPool &) = delete;
		WorkerPool &operator=(const WorkerPool &) = delete;
		WorkerPool(WorkerPool &&) noexcept = delete;
		WorkerPool &operator=(WorkerPool &&) noexcept = delete;

		// Process-wide pool for short CPU jobs. Construct a dedicated pool when a
		// subsystem needs isolated capacity or a different worker count.
		[[nodiscard]] static WorkerPool &global();

		template <typename TFunction, typename... TArguments>
			requires std::invocable<std::decay_t<TFunction>, std::decay_t<TArguments>...>
		[[nodiscard]] auto push(TFunction &&p_function, TArguments &&...p_arguments)
			-> Task<std::invoke_result_t<std::decay_t<TFunction>, std::decay_t<TArguments>...>>
		{
			using Result = std::invoke_result_t<std::decay_t<TFunction>, std::decay_t<TArguments>...>;
			auto invocation = [function = std::decay_t<TFunction>(std::forward<TFunction>(p_function)),
							   arguments = std::tuple<std::decay_t<TArguments>...>(std::forward<TArguments>(p_arguments)...)]() mutable -> Result {
				return std::apply(
					[&function](auto &...p_storedArguments) -> Result {
						return std::invoke(std::move(function), std::move(p_storedArguments)...);
					},
					arguments);
			};

			auto resultWork = std::make_shared<std::packaged_task<Result()>>(std::move(invocation));
			std::future<Result> result = resultWork->get_future();
			_push([resultWork]() {
				(*resultWork)();
			});
			return Task<Result>(std::move(result));
		}

		// Waits until every submitted job is complete. Inspect individual Task
		// handles when exceptions need to be observed.
		void wait();

		[[nodiscard]] std::size_t workerCount() const noexcept;
		[[nodiscard]] std::size_t unfinishedWorkCount() const;
	};
}
