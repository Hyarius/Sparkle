#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>

#include "spk_contract_provider.hpp"
#include "spk_thread_safe_contract.hpp"

TEST(ThreadSafeContractTest, StartsInvalid)
{
	spk::ThreadSafeContract<spk::ContractProvider<>::Contract> contract;

	EXPECT_FALSE(contract.isValid());
	EXPECT_NO_THROW(contract.resign());
	EXPECT_NO_THROW(contract.relinquish());
}

TEST(ThreadSafeContractTest, ResignUsesTheProvidedMutex)
{
	spk::ContractProvider<> provider;
	std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
	int callCount = 0;

	spk::ThreadSafeContract<spk::ContractProvider<>::Contract> contract(
		provider.subscribe([&callCount]()
		{
			++callCount;
		}),
		mutex);

	std::unique_lock<std::mutex> lock(*mutex);
	std::atomic<bool> workerStarted = false;

	std::jthread worker(
		[contract = std::move(contract), &workerStarted]() mutable
		{
			workerStarted.store(true);
			contract.resign();
		});

	while (workerStarted.load() == false)
	{
		std::this_thread::yield();
	}

	provider.trigger();
	EXPECT_EQ(callCount, 1);

	lock.unlock();
	worker.join();

	{
		std::scoped_lock providerLock(*mutex);
		provider.trigger();
	}

	EXPECT_EQ(callCount, 1);
	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ThreadSafeContractTest, DestructorResignsContract)
{
	spk::ContractProvider<> provider;
	std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
	int callCount = 0;

	{
		spk::ThreadSafeContract<spk::ContractProvider<>::Contract> contract(
			provider.subscribe([&callCount]()
			{
				++callCount;
			}),
			mutex);

		EXPECT_TRUE(contract.isValid());
	}

	{
		std::scoped_lock lock(*mutex);
		provider.trigger();
	}

	EXPECT_EQ(callCount, 0);
	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ThreadSafeContractTest, RelinquishDetachesWithoutResigning)
{
	spk::ContractProvider<> provider;
	std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
	int callCount = 0;

	spk::ThreadSafeContract<spk::ContractProvider<>::Contract> contract(
		provider.subscribe([&callCount]()
		{
			++callCount;
		}),
		mutex);

	contract.relinquish();

	{
		std::scoped_lock lock(*mutex);
		provider.trigger();
	}

	EXPECT_EQ(callCount, 1);
	EXPECT_FALSE(contract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ThreadSafeContractTest, BlockReturnsSynchronizedBlocker)
{
	spk::ContractProvider<> provider;
	std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
	int callCount = 0;

	spk::ThreadSafeContract<spk::ContractProvider<>::Contract> contract(
		provider.subscribe([&callCount]()
		{
			++callCount;
		}),
		mutex);

	auto blocker = contract.block();

	{
		std::scoped_lock lock(*mutex);
		provider.trigger();
	}

	EXPECT_EQ(callCount, 0);
	EXPECT_TRUE(blocker.isValid());

	blocker.release();

	{
		std::scoped_lock lock(*mutex);
		provider.trigger();
	}

	EXPECT_EQ(callCount, 1);
}
