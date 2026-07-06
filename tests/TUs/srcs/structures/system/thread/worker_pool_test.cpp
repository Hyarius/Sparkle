#include <gtest/gtest.h>

#include "structures/system/thread/spk_worker_pool.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

TEST(WorkerPoolTest, ExecutesSubmittedWorkAndExposesTaskCompletion)
{
	spk::WorkerPool pool(4);
	std::atomic_size_t executionCount = 0;
	std::vector<spk::WorkerPool::Task<void>> tasks;
	for (std::size_t index = 0; index < 64; ++index)
	{
		tasks.push_back(pool.push([&executionCount]() {
			executionCount.fetch_add(1, std::memory_order_relaxed);
		}));
	}

	pool.wait();
	EXPECT_EQ(executionCount.load(std::memory_order_relaxed), 64u);
	EXPECT_EQ(pool.unfinishedWorkCount(), 0u);
	for (spk::WorkerPool::Task<void> &task : tasks)
	{
		EXPECT_TRUE(task.valid());
		EXPECT_TRUE(task.isComplete());
		EXPECT_NO_THROW(task.obtain());
	}
}

TEST(WorkerPoolTest, TaskWaitRethrowsWorkerExceptionsWithoutStoppingThePool)
{
	spk::WorkerPool pool(2);
	spk::WorkerPool::Task<void> failing = pool.push([]() {
		throw std::runtime_error("task failed");
	});
	spk::WorkerPool::Task<void> succeeding = pool.push([]() {
	});

	EXPECT_THROW(failing.obtain(), std::runtime_error);
	EXPECT_NO_THROW(succeeding.obtain());
	EXPECT_NO_THROW(pool.push([]() {
						})
						.obtain());
}

TEST(WorkerPoolTest, DeducesArgumentsAndReturnsTypedResults)
{
	spk::WorkerPool pool(2);
	spk::WorkerPool::Task<int> result = pool.push(
		[](int p_left, int p_right) {
			return p_left + p_right;
		},
		4,
		8);

	EXPECT_EQ(result.obtain(), 12);
}

TEST(WorkerPoolTest, SupportsMoveOnlyResults)
{
	spk::WorkerPool pool(1);
	spk::WorkerPool::Task<std::unique_ptr<int>> result = pool.push([]() {
		return std::make_unique<int>(42);
	});

	std::unique_ptr<int> value = result.obtain();
	ASSERT_NE(value, nullptr);
	EXPECT_EQ(*value, 42);
}

TEST(WorkerPoolTest, RunsWorkOnPersistentWorkerThreads)
{
	spk::WorkerPool pool(1);
	const std::thread::id caller = std::this_thread::get_id();
	std::thread::id firstWorker;
	std::thread::id secondWorker;

	pool.push([&firstWorker]() {
			firstWorker = std::this_thread::get_id();
		})
		.obtain();
	pool.push([&secondWorker]() {
			secondWorker = std::this_thread::get_id();
		})
		.obtain();

	EXPECT_NE(firstWorker, caller);
	EXPECT_EQ(firstWorker, secondWorker);
	EXPECT_EQ(pool.workerCount(), 1u);
}

TEST(WorkerPoolTest, DestructionDrainsAlreadySubmittedWork)
{
	std::atomic_bool executed = false;
	spk::WorkerPool::Task<void> task;
	{
		spk::WorkerPool pool(1);
		task = pool.push([&executed]() {
			executed.store(true, std::memory_order_relaxed);
		});
	}

	EXPECT_TRUE(executed.load(std::memory_order_relaxed));
	EXPECT_TRUE(task.isComplete());
	EXPECT_NO_THROW(task.obtain());
}

TEST(WorkerPoolTest, GlobalPoolIsStableAndHasWorkers)
{
	spk::WorkerPool &first = spk::WorkerPool::global();
	spk::WorkerPool &second = spk::WorkerPool::global();

	EXPECT_EQ(&first, &second);
	EXPECT_GT(first.workerCount(), 0u);
	EXPECT_LE(first.workerCount(), spk::WorkerPool::AutomaticWorkerLimit);
}
