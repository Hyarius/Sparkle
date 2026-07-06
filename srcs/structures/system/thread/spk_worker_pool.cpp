#include "structures/system/thread/spk_worker_pool.hpp"

#include <algorithm>
#include <stdexcept>

namespace spk
{
	WorkerPool::WorkerPool(std::size_t p_workerCount)
	{
		const std::size_t workerCount = p_workerCount == 0
											? std::min(AutomaticWorkerLimit, std::max<std::size_t>(1, std::thread::hardware_concurrency()))
											: p_workerCount;
		_workers.reserve(workerCount);
		for (std::size_t index = 0; index < workerCount; ++index)
		{
			_workers.emplace_back([this](std::stop_token p_stopToken) {
				_run(p_stopToken);
			});
		}
	}

	WorkerPool::~WorkerPool()
	{
		{
			std::scoped_lock lock(_mutex);
			_acceptingWork = false;
		}
		for (std::jthread &worker : _workers)
		{
			worker.request_stop();
		}
		_workAvailable.notify_all();
		_workers.clear();
	}

	WorkerPool &WorkerPool::global()
	{
		static WorkerPool instance;
		return instance;
	}

	void WorkerPool::_push(std::function<void()> p_work)
	{
		{
			std::scoped_lock lock(_mutex);
			if (_acceptingWork == false)
			{
				throw std::runtime_error("WorkerPool no longer accepts work");
			}
			_queue.push_back(Work{.function = std::move(p_work)});
			++_unfinishedWorkCount;
		}
		_workAvailable.notify_one();
	}

	void WorkerPool::_run(std::stop_token p_stopToken)
	{
		while (true)
		{
			Work work;
			{
				std::unique_lock lock(_mutex);
				_workAvailable.wait(lock, p_stopToken, [this]() {
					return _queue.empty() == false;
				});
				if (_queue.empty())
				{
					if (p_stopToken.stop_requested())
					{
						return;
					}
					continue;
				}
				work = std::move(_queue.front());
				_queue.pop_front();
			}

			work.function();

			{
				std::scoped_lock lock(_mutex);
				--_unfinishedWorkCount;
			}
			_workCompleted.notify_all();
		}
	}

	void WorkerPool::wait()
	{
		std::unique_lock lock(_mutex);
		_workCompleted.wait(lock, [this]() {
			return _unfinishedWorkCount == 0;
		});
	}

	std::size_t WorkerPool::workerCount() const noexcept
	{
		return _workers.size();
	}

	std::size_t WorkerPool::unfinishedWorkCount() const
	{
		std::scoped_lock lock(_mutex);
		return _unfinishedWorkCount;
	}
}
