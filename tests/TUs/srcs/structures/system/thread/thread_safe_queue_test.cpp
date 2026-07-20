#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "structures/system/thread/spk_thread_safe_queue.hpp"

TEST(ThreadSafeQueueTest, StartsEmptyWithZeroSize)
{
	spk::ThreadSafeQueue<int> queue;

	EXPECT_TRUE(queue.empty());
	EXPECT_EQ(queue.size(), 0u);
	EXPECT_FALSE(queue.tryPop().has_value());
}

TEST(ThreadSafeQueueTest, PushAndTryPopPreserveFifoOrder)
{
	spk::ThreadSafeQueue<int> queue;
	queue.push(1);
	queue.push(2);
	queue.push(3);
	EXPECT_FALSE(queue.empty());
	EXPECT_EQ(queue.size(), 3u);

	for (int expectedValue = 1; expectedValue <= 3; ++expectedValue)
	{
		std::optional<int> value = queue.tryPop();
		ASSERT_TRUE(value.has_value());
		EXPECT_EQ(*value, expectedValue);
		EXPECT_EQ(queue.size(), static_cast<spk::ThreadSafeQueue<int>::SizeType>(3 - expectedValue));
	}

	EXPECT_TRUE(queue.empty());
	EXPECT_EQ(queue.size(), 0u);
}

TEST(ThreadSafeQueueTest, EmplaceConstructsValueFromArguments)
{
	struct Value
	{
		Value(int p_number, std::unique_ptr<int> p_payload) :
			number(p_number),
			payload(std::move(p_payload))
		{
		}

		Value(const Value &) = delete;
		Value &operator=(const Value &) = delete;
		Value(Value &&) noexcept = default;
		Value &operator=(Value &&) = delete;

		int number;
		std::unique_ptr<int> payload;
	};

	spk::ThreadSafeQueue<Value> queue;
	queue.emplace(42, std::make_unique<int>(99));

	std::optional<Value> value = queue.tryPop();
	ASSERT_TRUE(value.has_value());
	EXPECT_EQ(value->number, 42);
	ASSERT_NE(value->payload, nullptr);
	EXPECT_EQ(*value->payload, 99);
}

TEST(ThreadSafeQueueTest, SupportsMoveOnlyValues)
{
	spk::ThreadSafeQueue<std::unique_ptr<int>> queue;
	queue.push(std::make_unique<int>(42));

	std::optional<std::unique_ptr<int>> value = queue.tryPop();
	ASSERT_TRUE(value.has_value());
	ASSERT_NE(*value, nullptr);
	EXPECT_EQ(**value, 42);
}

TEST(ThreadSafeQueueTest, CanBeReusedAfterBecomingEmpty)
{
	spk::ThreadSafeQueue<int> queue;
	queue.push(1);
	EXPECT_EQ(*queue.tryPop(), 1);
	queue.push(2);
	EXPECT_EQ(*queue.tryPop(), 2);
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueueTest, DrainPreservesFifoOrderAndAllowsReuse)
{
	spk::ThreadSafeQueue<int> queue;
	queue.push(10);
	queue.push(20);
	queue.push(30);

	std::deque<int> values = queue.drain();
	EXPECT_EQ(values, (std::deque<int>{10, 20, 30}));
	EXPECT_TRUE(queue.empty());
	EXPECT_TRUE(queue.drain().empty());

	queue.push(40);
	EXPECT_EQ(*queue.tryPop(), 40);
}

TEST(ThreadSafeQueueTest, ClearEmptiesQueueAndAllowsReuse)
{
	spk::ThreadSafeQueue<int> queue;
	queue.push(1);
	queue.push(2);
	queue.clear();

	EXPECT_TRUE(queue.empty());
	EXPECT_EQ(queue.size(), 0u);
	EXPECT_FALSE(queue.tryPop().has_value());

	queue.push(3);
	EXPECT_EQ(*queue.tryPop(), 3);
}

TEST(ThreadSafeQueueTest, MultipleProducersAndConsumersRetrieveEachValueExactlyOnce)
{
	spk::ThreadSafeQueue<int> queue;
	constexpr int producerCount = 4;
	constexpr int consumerCount = 4;
	constexpr int valuesPerProducer = 1000;
	constexpr int valueCount = producerCount * valuesPerProducer;

	std::atomic<int> completedProducers = 0;
	std::atomic<int> consumedCount = 0;
	std::vector<std::atomic<int>> retrievalCounts(valueCount);

	std::vector<std::thread> producers;
	for (int producerIndex = 0; producerIndex < producerCount; ++producerIndex)
	{
		producers.emplace_back(
			[&queue, &completedProducers, producerIndex]()
			{
				for (int valueIndex = 0; valueIndex < valuesPerProducer; ++valueIndex)
				{
					queue.push((producerIndex * valuesPerProducer) + valueIndex);
				}
				++completedProducers;
			});
	}

	std::vector<std::thread> consumers;
	for (int consumerIndex = 0; consumerIndex < consumerCount; ++consumerIndex)
	{
		consumers.emplace_back(
			[&queue, &completedProducers, &consumedCount, &retrievalCounts]()
			{
				while (completedProducers.load() < producerCount || !queue.empty())
				{
					if (std::optional<int> value = queue.tryPop())
					{
						++retrievalCounts[*value];
						++consumedCount;
					}
				}
			});
	}

	for (std::thread &producer : producers)
	{
		producer.join();
	}
	for (std::thread &consumer : consumers)
	{
		consumer.join();
	}

	EXPECT_EQ(consumedCount.load(), valueCount);
	EXPECT_TRUE(queue.empty());
	for (const std::atomic<int> &retrievalCount : retrievalCounts)
	{
		EXPECT_EQ(retrievalCount.load(), 1);
	}
}

TEST(ThreadSafeQueueTest, DrainAndPushesLeaveEveryValueAvailableExactlyOnce)
{
	spk::ThreadSafeQueue<int> queue;
	constexpr int valueCount = 1000;
	std::atomic<bool> producerDone = false;
	std::vector<int> drainedValues;

	std::thread producer(
		[&queue, &producerDone]()
		{
			for (int value = 0; value < valueCount; ++value)
			{
				queue.push(value);
			}
			producerDone.store(true);
		});

	std::thread drainer(
		[&queue, &producerDone, &drainedValues]()
		{
			while (!producerDone.load() || !queue.empty())
			{
				std::deque<int> batch = queue.drain();
				drainedValues.insert(drainedValues.end(), batch.begin(), batch.end());
			}
		});

	producer.join();
	drainer.join();

	ASSERT_EQ(drainedValues.size(), valueCount);
	std::vector<int> retrievalCounts(valueCount, 0);
	for (const int value : drainedValues)
	{
		ASSERT_GE(value, 0);
		ASSERT_LT(value, valueCount);
		++retrievalCounts[value];
	}
	for (const int retrievalCount : retrievalCounts)
	{
		EXPECT_EQ(retrievalCount, 1);
	}
}

TEST(ThreadSafeQueueTest, ClearDestroysValuesOutsideTheQueueLock)
{
	struct DestructionState
	{
		std::mutex mutex;
		std::condition_variable condition;
		bool destructionStarted = false;
		bool releaseDestruction = false;
	};

	struct BlockingDestructorValue
	{
		explicit BlockingDestructorValue(std::shared_ptr<DestructionState> p_state = nullptr) :
			state(std::move(p_state))
		{
		}

		BlockingDestructorValue(const BlockingDestructorValue &) = delete;
		BlockingDestructorValue &operator=(const BlockingDestructorValue &) = delete;

		BlockingDestructorValue(BlockingDestructorValue &&p_other) noexcept :
			state(std::move(p_other.state))
		{
		}

		BlockingDestructorValue &operator=(BlockingDestructorValue &&) = delete;

		~BlockingDestructorValue()
		{
			if (state == nullptr)
			{
				return;
			}

			std::unique_lock lock(state->mutex);
			state->destructionStarted = true;
			state->condition.notify_all();
			state->condition.wait(lock, [this] { return state->releaseDestruction; });
		}

		std::shared_ptr<DestructionState> state;
	};

	spk::ThreadSafeQueue<BlockingDestructorValue> queue;
	std::shared_ptr<DestructionState> state = std::make_shared<DestructionState>();
	queue.emplace(state);

	std::thread clearer([&queue] { queue.clear(); });
	{
		std::unique_lock lock(state->mutex);
		state->condition.wait(lock, [&state] { return state->destructionStarted; });
	}

	queue.emplace();

	{
		std::lock_guard lock(state->mutex);
		state->releaseDestruction = true;
	}
	state->condition.notify_all();
	clearer.join();
	EXPECT_EQ(queue.size(), 1u);
}
