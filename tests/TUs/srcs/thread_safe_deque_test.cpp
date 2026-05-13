#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "spk_thread_safe_deque.hpp"

TEST(ThreadSafeDequeTest, StartsEmpty)
{
	spk::ThreadSafeDeque<int> queue;

	EXPECT_TRUE(queue.empty());
	EXPECT_EQ(queue.size(), 0u);
}

TEST(ThreadSafeDequeTest, PushBackAndPopFrontPreserveFifoOrder)
{
	spk::ThreadSafeDeque<int> queue;

	queue.pushBack(1);
	queue.pushBack(2);
	queue.pushBack(3);

	int value = 0;
	ASSERT_TRUE(queue.popFront(value));
	EXPECT_EQ(value, 1);
	ASSERT_TRUE(queue.popFront(value));
	EXPECT_EQ(value, 2);
	ASSERT_TRUE(queue.popFront(value));
	EXPECT_EQ(value, 3);
	EXPECT_FALSE(queue.popFront(value));
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, SupportsMoveOnlyValues)
{
	spk::ThreadSafeDeque<std::unique_ptr<int>> queue;

	queue.pushBack(std::make_unique<int>(42));

	std::unique_ptr<int> value;
	ASSERT_TRUE(queue.popFront(value));
	ASSERT_NE(value, nullptr);
	EXPECT_EQ(*value, 42);
}

TEST(ThreadSafeDequeTest, DrainReturnsAllValuesAndClearsQueue)
{
	spk::ThreadSafeDeque<int> queue;

	queue.pushBack(10);
	queue.pushBack(20);
	queue.pushBack(30);

	std::vector<int> values = queue.drain();

	ASSERT_EQ(values.size(), 3u);
	EXPECT_EQ(values[0], 10);
	EXPECT_EQ(values[1], 20);
	EXPECT_EQ(values[2], 30);
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, ClearRemovesQueuedValues)
{
	spk::ThreadSafeDeque<int> queue;

	queue.pushBack(1);
	queue.pushBack(2);
	queue.clear();

	EXPECT_TRUE(queue.empty());

	int value = 0;
	EXPECT_FALSE(queue.popFront(value));
}

TEST(ThreadSafeDequeTest, ConcurrentProducersCanPushSafely)
{
	spk::ThreadSafeDeque<int> queue;
	constexpr int producerCount = 4;
	constexpr int valueCountPerProducer = 250;

	std::vector<std::thread> producers;
	for (int producerIndex = 0; producerIndex < producerCount; ++producerIndex)
	{
		producers.emplace_back(
			[&queue, producerIndex]()
			{
				for (int valueIndex = 0; valueIndex < valueCountPerProducer; ++valueIndex)
				{
					queue.pushBack((producerIndex * valueCountPerProducer) + valueIndex);
				}
			});
	}

	for (std::thread& producer : producers)
	{
		producer.join();
	}

	EXPECT_EQ(queue.size(), static_cast<size_t>(producerCount * valueCountPerProducer));
	EXPECT_EQ(queue.drain().size(), static_cast<size_t>(producerCount * valueCountPerProducer));
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, ProducerAndConsumerCanRunConcurrently)
{
	spk::ThreadSafeDeque<int> queue;
	constexpr int valueCount = 1000;
	std::atomic<int> consumedCount = 0;
	std::atomic<bool> producerDone = false;

	std::thread producer(
		[&queue, &producerDone]()
		{
			for (int i = 0; i < valueCount; ++i)
			{
				queue.pushBack(i);
			}
			producerDone.store(true);
		});

	std::thread consumer(
		[&queue, &consumedCount, &producerDone]()
		{
			int value = 0;
			while (producerDone.load() == false || queue.empty() == false)
			{
				if (queue.popFront(value) == true)
				{
					++consumedCount;
				}
			}
		});

	producer.join();
	consumer.join();

	EXPECT_EQ(consumedCount.load(), valueCount);
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, MultipleProducersAndConsumersCanRunConcurrently)
{
	spk::ThreadSafeDeque<int> queue;

	constexpr int producerCount = 4;
	constexpr int consumerCount = 4;
	constexpr int valueCountPerProducer = 1000;
	constexpr int totalValueCount = producerCount * valueCountPerProducer;

	std::atomic<int> consumedCount = 0;
	std::atomic<int> completedProducerCount = 0;

	std::vector<std::atomic<int>> valueCounters(totalValueCount);
	for (std::atomic<int>& counter : valueCounters)
	{
		counter.store(0);
	}

	std::vector<std::thread> producers;
	for (int producerIndex = 0; producerIndex < producerCount; ++producerIndex)
	{
		producers.emplace_back(
			[&queue, &completedProducerCount, producerIndex]()
			{
				for (int valueIndex = 0; valueIndex < valueCountPerProducer; ++valueIndex)
				{
					queue.pushBack((producerIndex * valueCountPerProducer) + valueIndex);
				}

				++completedProducerCount;
			});
	}

	std::vector<std::thread> consumers;
	for (int consumerIndex = 0; consumerIndex < consumerCount; ++consumerIndex)
	{
		consumers.emplace_back(
			[&queue, &consumedCount, &completedProducerCount, &valueCounters]()
			{
				int value = 0;

				while (completedProducerCount.load() < producerCount || queue.empty() == false)
				{
					if (queue.popFront(value) == true)
					{
						++valueCounters[value];
						++consumedCount;
					}
				}
			});
	}

	for (std::thread& producer : producers)
	{
		producer.join();
	}

	for (std::thread& consumer : consumers)
	{
		consumer.join();
	}

	EXPECT_EQ(consumedCount.load(), totalValueCount);
	EXPECT_TRUE(queue.empty());

	for (const std::atomic<int>& counter : valueCounters)
	{
		EXPECT_EQ(counter.load(), 1);
	}
}

TEST(ThreadSafeDequeTest, DrainPreservesFifoOrder)
{
	spk::ThreadSafeDeque<int> queue;

	for (int i = 0; i < 100; ++i)
	{
		queue.pushBack(i);
	}

	std::vector<int> values = queue.drain();

	ASSERT_EQ(values.size(), 100u);

	for (int i = 0; i < 100; ++i)
	{
		EXPECT_EQ(values[i], i);
	}
}

TEST(ThreadSafeDequeTest, DrainSupportsMoveOnlyValues)
{
	spk::ThreadSafeDeque<std::unique_ptr<int>> queue;

	queue.pushBack(std::make_unique<int>(1));
	queue.pushBack(std::make_unique<int>(2));
	queue.pushBack(std::make_unique<int>(3));

	std::vector<std::unique_ptr<int>> values = queue.drain();

	ASSERT_EQ(values.size(), 3u);
	ASSERT_NE(values[0], nullptr);
	ASSERT_NE(values[1], nullptr);
	ASSERT_NE(values[2], nullptr);

	EXPECT_EQ(*values[0], 1);
	EXPECT_EQ(*values[1], 2);
	EXPECT_EQ(*values[2], 3);
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, DrainCanRunWhileProducerPushes)
{
	spk::ThreadSafeDeque<int> queue;

	constexpr int valueCount = 1000;
	std::atomic<bool> producerDone = false;
	std::atomic<int> drainedCount = 0;

	std::thread producer(
		[&queue, &producerDone]()
		{
			for (int i = 0; i < valueCount; ++i)
			{
				queue.pushBack(i);
			}

			producerDone.store(true);
		});

	std::thread drainer(
		[&queue, &producerDone, &drainedCount]()
		{
			while (producerDone.load() == false || queue.empty() == false)
			{
				std::vector<int> values = queue.drain();
				drainedCount += static_cast<int>(values.size());
			}
		});

	producer.join();
	drainer.join();

	EXPECT_EQ(drainedCount.load(), valueCount);
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, EmplaceBackConstructsValueFromArguments)
{
	struct TestValue
	{
		int integerValue;
		std::string stringValue;

		TestValue(int p_integerValue, std::string p_stringValue) :
			integerValue(p_integerValue),
			stringValue(std::move(p_stringValue))
		{
		}
	};

	spk::ThreadSafeDeque<TestValue> queue;

	queue.emplaceBack(42, "Hello");

	TestValue value(0, "");

	ASSERT_TRUE(queue.popFront(value));
	EXPECT_EQ(value.integerValue, 42);
	EXPECT_EQ(value.stringValue, "Hello");
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, EmplaceBackSupportsMoveOnlyValues)
{
	spk::ThreadSafeDeque<std::unique_ptr<int>> queue;

	queue.emplaceBack(std::make_unique<int>(12));
	queue.emplaceBack(std::make_unique<int>(34));

	std::vector<std::unique_ptr<int>> values = queue.drain();

	ASSERT_EQ(values.size(), 2u);
	ASSERT_NE(values[0], nullptr);
	ASSERT_NE(values[1], nullptr);

	EXPECT_EQ(*values[0], 12);
	EXPECT_EQ(*values[1], 34);
	EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeDequeTest, EmplaceBackSupportsInPlaceConstruction)
{
	struct MoveOnlyValue
	{
		int id;
		std::unique_ptr<int> payload;

		MoveOnlyValue(int p_id, std::unique_ptr<int> p_payload) :
			id(p_id),
			payload(std::move(p_payload))
		{
		}

		MoveOnlyValue(const MoveOnlyValue&) = delete;
		MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;

		MoveOnlyValue(MoveOnlyValue&&) noexcept = default;
		MoveOnlyValue& operator=(MoveOnlyValue&&) noexcept = default;
	};

	spk::ThreadSafeDeque<MoveOnlyValue> queue;

	queue.emplaceBack(7, std::make_unique<int>(99));

	std::vector<MoveOnlyValue> values = queue.drain();

	ASSERT_EQ(values.size(), 1u);
	EXPECT_EQ(values[0].id, 7);
	ASSERT_NE(values[0].payload, nullptr);
	EXPECT_EQ(*values[0].payload, 99);
	EXPECT_TRUE(queue.empty());
}