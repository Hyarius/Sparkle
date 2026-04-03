#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "spk_contract_provider.hpp"

TEST(ContractProviderTest, NewProviderStartsEmpty)
{
	spk::ContractProvider provider;

	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ContractProviderTest, SubscribeAddsValidContractAndIncreasesCount)
{
	spk::ContractProvider provider;

	auto contract = provider.subscribe([]() {});

	EXPECT_TRUE(contract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, TriggerCallsSingleSubscribedCallback)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto contract = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	ASSERT_TRUE(contract.isValid());

	provider.trigger();

	EXPECT_EQ(callCount, 1);
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, TriggerCallsAllSubscribedCallbacksInSubscriptionOrder)
{
	spk::ContractProvider provider;
	std::vector<int> callOrder;

	auto first = provider.subscribe([&callOrder]()
	{
		callOrder.push_back(1);
	});

	auto second = provider.subscribe([&callOrder]()
	{
		callOrder.push_back(2);
	});

	auto third = provider.subscribe([&callOrder]()
	{
		callOrder.push_back(3);
	});

	ASSERT_TRUE(first.isValid());
	ASSERT_TRUE(second.isValid());
	ASSERT_TRUE(third.isValid());

	provider.trigger();

	std::vector<int> expected = {1, 2, 3};
	EXPECT_EQ(callOrder, expected);
}

TEST(ContractProviderTest, MultipleTriggersCallCallbacksMultipleTimes)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto contract = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	provider.trigger();
	provider.trigger();
	provider.trigger();

	EXPECT_EQ(callCount, 3);
	EXPECT_TRUE(contract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, ProviderBlockPreventsTriggerWhileTokenLives)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto contract = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	spk::ContractProvider::Blocker blocker = provider.block();

	EXPECT_TRUE(provider.isBlocked());

	provider.trigger();
	EXPECT_EQ(callCount, 0);

	blocker.release();

	EXPECT_FALSE(provider.isBlocked());

	provider.trigger();
	EXPECT_EQ(callCount, 1);
	EXPECT_TRUE(contract.isValid());
}

TEST(ContractProviderTest, NestedProviderBlocksRequireAllTokensReleased)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto contract = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	auto firstBlocker = provider.block();
	auto secondBlocker = provider.block();

	ASSERT_TRUE(provider.isBlocked());

	provider.trigger();
	EXPECT_EQ(callCount, 0);

	firstBlocker.release();
	EXPECT_TRUE(provider.isBlocked());

	provider.trigger();
	EXPECT_EQ(callCount, 0);

	secondBlocker.release();
	EXPECT_FALSE(provider.isBlocked());

	provider.trigger();
	EXPECT_EQ(callCount, 1);
}

TEST(ContractProviderTest, ContractBlockPreventsOnlyThatCallback)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;

	auto first = provider.subscribe([&firstCount]()
	{
		++firstCount;
	});

	auto second = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	auto blocker = first.block();

	provider.trigger();

	EXPECT_EQ(firstCount, 0);
	EXPECT_EQ(secondCount, 1);
	EXPECT_TRUE(first.isValid());
	EXPECT_TRUE(second.isValid());

	blocker.release();

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 2);
}

TEST(ContractProviderTest, BlockingInvalidContractReturnsInvalidBlocker)
{
	spk::ContractProvider::Contract contract;

	auto blocker = contract.block();

	EXPECT_FALSE(blocker.isValid());
}

TEST(ContractProviderTest, ContractResignInvalidatesContractAndRemovesCallbackOnCleanup)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto contract = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	ASSERT_TRUE(contract.isValid());
	ASSERT_EQ(provider.nbContracts(), 1u);

	contract.resign();

	EXPECT_FALSE(contract.isValid());

	provider.trigger();

	EXPECT_EQ(callCount, 0);
	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ContractProviderTest, ContractDestructorResignsAutomatically)
{
	spk::ContractProvider provider;
	int callCount = 0;

	{
		auto contract = provider.subscribe([&callCount]()
		{
			++callCount;
		});

		ASSERT_TRUE(contract.isValid());
		ASSERT_EQ(provider.nbContracts(), 1u);
	}

	provider.trigger();

	EXPECT_EQ(callCount, 0);
	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ContractProviderTest, RelinquishDetachesContractWithoutRemovingCallback)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto contract = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	ASSERT_TRUE(contract.isValid());
	ASSERT_EQ(provider.nbContracts(), 1u);

	contract.relinquish();

	EXPECT_FALSE(contract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);

	provider.trigger();

	EXPECT_EQ(callCount, 1);
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, MovedContractTransfersOwnershipOfSubscription)
{
	spk::ContractProvider provider;
	int callCount = 0;

	auto source = provider.subscribe([&callCount]()
	{
		++callCount;
	});

	ASSERT_TRUE(source.isValid());

	spk::ContractProvider::Contract destination(std::move(source));

	EXPECT_FALSE(source.isValid());
	EXPECT_TRUE(destination.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);

	provider.trigger();

	EXPECT_EQ(callCount, 1);

	destination.resign();

	EXPECT_FALSE(destination.isValid());
	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ContractProviderTest, MoveAssignedContractTransfersOwnershipOfSubscription)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;

	auto first = provider.subscribe([&firstCount]()
	{
		++firstCount;
	});

	auto second = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	ASSERT_TRUE(first.isValid());
	ASSERT_TRUE(second.isValid());
	ASSERT_EQ(provider.nbContracts(), 2u);

	second = std::move(first);

	EXPECT_FALSE(first.isValid());
	EXPECT_TRUE(second.isValid());

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 0);
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, InvalidateAllContractsPreventsFurtherCalls)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;
	int thirdCount = 0;

	auto first = provider.subscribe([&firstCount]()
	{
		++firstCount;
	});

	auto second = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	auto third = provider.subscribe([&thirdCount]()
	{
		++thirdCount;
	});

	ASSERT_EQ(provider.nbContracts(), 3u);

	provider.invalidateAllContracts();

	EXPECT_FALSE(first.isValid());
	EXPECT_FALSE(second.isValid());
	EXPECT_FALSE(third.isValid());
	EXPECT_EQ(provider.nbContracts(), 0u);

	provider.trigger();

	EXPECT_EQ(firstCount, 0);
	EXPECT_EQ(secondCount, 0);
	EXPECT_EQ(thirdCount, 0);
}

TEST(ContractProviderTest, CallbackCanResignItselfDuringTrigger)
{
	spk::ContractProvider provider;
	int selfCount = 0;
	int otherCount = 0;

	spk::ContractProvider::Contract selfContract;
	selfContract = provider.subscribe([&selfCount, &selfContract]()
	{
		++selfCount;
		selfContract.resign();
	});

	auto otherContract = provider.subscribe([&otherCount]()
	{
		++otherCount;
	});

	provider.trigger();

	EXPECT_EQ(selfCount, 1);
	EXPECT_EQ(otherCount, 1);
	EXPECT_FALSE(selfContract.isValid());
	EXPECT_TRUE(otherContract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);

	provider.trigger();

	EXPECT_EQ(selfCount, 1);
	EXPECT_EQ(otherCount, 2);
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, CallbackCanResignAnotherContractBeforeItsTurn)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;

	spk::ContractProvider::Contract secondContract;

	auto firstContract = provider.subscribe([&firstCount, &secondContract]()
	{
		++firstCount;
		secondContract.resign();
	});

	secondContract = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 0);
	EXPECT_TRUE(firstContract.isValid());
	EXPECT_FALSE(secondContract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, CallbackCanRelinquishAnotherContractWithoutRemovingIt)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;

	spk::ContractProvider::Contract secondContract;

	auto firstContract = provider.subscribe([&firstCount, &secondContract]()
	{
		++firstCount;
		secondContract.relinquish();
	});

	secondContract = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
	EXPECT_TRUE(firstContract.isValid());
	EXPECT_FALSE(secondContract.isValid());
	EXPECT_EQ(provider.nbContracts(), 2u);

	provider.trigger();

	EXPECT_EQ(firstCount, 2);
	EXPECT_EQ(secondCount, 2);
	EXPECT_EQ(provider.nbContracts(), 2u);
}

TEST(ContractProviderTest, CallbackCanSubscribeNewCallbackDuringTriggerButNewOneDoesNotRunImmediately)
{
	spk::ContractProvider provider;
	std::vector<std::string> log;

	spk::ContractProvider::Contract lateContract;

	auto first = provider.subscribe([&provider, &log, &lateContract]()
	{
		log.push_back("first");
		if (lateContract.isValid() == false)
		{
			lateContract = provider.subscribe([&log]()
			{
				log.push_back("late");
			});
		}
	});

	auto second = provider.subscribe([&log]()
	{
		log.push_back("second");
	});

	EXPECT_TRUE(first.isValid());
	EXPECT_TRUE(second.isValid());
	EXPECT_FALSE(lateContract.isValid());
	EXPECT_EQ(provider.nbContracts(), 2u);

	provider.trigger();

	std::vector<std::string> expectedAfterFirstTrigger = {"first", "second"};
	EXPECT_EQ(log, expectedAfterFirstTrigger);
	EXPECT_TRUE(first.isValid());
	EXPECT_TRUE(second.isValid());
	EXPECT_TRUE(lateContract.isValid());
	EXPECT_EQ(provider.nbContracts(), 3u);

	provider.trigger();

	std::vector<std::string> expectedAfterSecondTrigger = {"first", "second", "first", "second", "late"};
	EXPECT_EQ(log, expectedAfterSecondTrigger);
}

TEST(ContractProviderTest, ReentrantTriggerIsIgnored)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;

	auto first = provider.subscribe([&provider, &firstCount]()
	{
		++firstCount;
		provider.trigger();
	});

	auto second = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
	EXPECT_TRUE(first.isValid());
	EXPECT_TRUE(second.isValid());
}

TEST(ContractProviderTest, CallbackCanBlockProviderTemporarilyInsideTrigger)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;

	auto first = provider.subscribe([&provider, &firstCount]()
	{
		++firstCount;
		auto blocker = provider.block();
		provider.trigger();
	});

	auto second = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
	EXPECT_FALSE(provider.isBlocked());
}

TEST(ContractProviderTest, CallbackCanBlockAnotherContractBeforeItsTurn)
{
	spk::ContractProvider provider;
	int firstCount = 0;
	int secondCount = 0;
	int thirdCount = 0;

	spk::ContractProvider::Contract secondContract;

	auto firstContract = provider.subscribe([&firstCount, &secondContract]()
	{
		++firstCount;
		auto blocker = secondContract.block();
	});

	secondContract = provider.subscribe([&secondCount]()
	{
		++secondCount;
	});

	auto thirdContract = provider.subscribe([&thirdCount]()
	{
		++thirdCount;
	});

	provider.trigger();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
	EXPECT_EQ(thirdCount, 1);

	EXPECT_TRUE(firstContract.isValid());
	EXPECT_TRUE(secondContract.isValid());
	EXPECT_TRUE(thirdContract.isValid());
}

TEST(ContractProviderTest, PersistentlyBlockedContractRemainsSkippedAcrossTriggers)
{
	spk::ContractProvider provider;
	int blockedCount = 0;
	int freeCount = 0;

	auto blockedContract = provider.subscribe([&blockedCount]()
	{
		++blockedCount;
	});

	auto freeContract = provider.subscribe([&freeCount]()
	{
		++freeCount;
	});

	auto blocker = blockedContract.block();

	provider.trigger();
	provider.trigger();
	provider.trigger();

	EXPECT_EQ(blockedCount, 0);
	EXPECT_EQ(freeCount, 3);

	blocker.release();

	provider.trigger();

	EXPECT_EQ(blockedCount, 1);
	EXPECT_EQ(freeCount, 4);
}

TEST(ContractProviderTest, NbContractsCountsOnlyValidContracts)
{
	spk::ContractProvider provider;

	auto first = provider.subscribe([]() {});
	auto second = provider.subscribe([]() {});
	auto third = provider.subscribe([]() {});

	ASSERT_EQ(provider.nbContracts(), 3u);

	second.resign();
	EXPECT_EQ(provider.nbContracts(), 2u);

	third.relinquish();
	EXPECT_EQ(provider.nbContracts(), 2u);

	provider.invalidateAllContracts();
	EXPECT_EQ(provider.nbContracts(), 0u);

	EXPECT_FALSE(first.isValid());
	EXPECT_FALSE(second.isValid());
	EXPECT_FALSE(third.isValid());
}

TEST(ContractProviderTest, EmptyCallbackObjectIsCountedButSkippedOnTrigger)
{
	spk::ContractProvider provider;
	int validCount = 0;

	spk::ContractProvider::Callback emptyCallback;
	auto emptyContract = provider.subscribe(std::move(emptyCallback));
	auto validContract = provider.subscribe([&validCount]()
	{
		++validCount;
	});

	EXPECT_FALSE(emptyContract.isValid());
	EXPECT_TRUE(validContract.isValid());
	EXPECT_EQ(provider.nbContracts(), 1u);

	provider.trigger();

	EXPECT_EQ(validCount, 1);
	EXPECT_EQ(provider.nbContracts(), 1u);
}

TEST(ContractProviderTest, ResignedEntriesAreCleanedAfterTrigger)
{
	spk::ContractProvider provider;
	int count = 0;

	auto first = provider.subscribe([&count]()
	{
		++count;
	});

	auto second = provider.subscribe([&count]()
	{
		++count;
	});

	auto third = provider.subscribe([&count]()
	{
		++count;
	});

	ASSERT_EQ(provider.nbContracts(), 3u);

	second.resign();

	EXPECT_EQ(provider.nbContracts(), 2u);

	provider.trigger();

	EXPECT_EQ(count, 2);
	EXPECT_EQ(provider.nbContracts(), 2u);

	first.resign();
	third.resign();

	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ContractProviderTest, InvalidateAllContractsIsSafeOnEmptyProvider)
{
	spk::ContractProvider provider;

	EXPECT_NO_THROW(provider.invalidateAllContracts());
	EXPECT_EQ(provider.nbContracts(), 0u);
}

TEST(ContractProviderTest, TriggerIsSafeOnEmptyProvider)
{
	spk::ContractProvider provider;

	EXPECT_NO_THROW(provider.trigger());
	EXPECT_EQ(provider.nbContracts(), 0u);
}