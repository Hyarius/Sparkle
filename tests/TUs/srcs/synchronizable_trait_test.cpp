#include <gtest/gtest.h>

#include "spk_synchronizable_trait.hpp"

namespace
{
	class TestSynchronizable : public spk::SynchronizableTrait
	{
	private:
		int _synchronizeCount = 0;

	protected:
		void _synchronize() override
		{
			++_synchronizeCount;
		}

	public:
		TestSynchronizable() = default;
		~TestSynchronizable() = default;

		int synchronizeCount() const
		{
			return _synchronizeCount;
		}
	};
}

TEST(SynchronizableTraitTest, FreshObjectDoesNotNeedSynchronization)
{
	TestSynchronizable object;

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 0);
}

TEST(SynchronizableTraitTest, RequestSynchronizationMarksObjectAsNeedingSynchronization)
{
	TestSynchronizable object;

	object.requestSynchronization();

	EXPECT_TRUE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 0);
}

TEST(SynchronizableTraitTest, SynchronizeDoesNothingWhenNoSynchronizationIsPending)
{
	TestSynchronizable object;

	object.synchronize();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 0);
}

TEST(SynchronizableTraitTest, SynchronizeCallsImplementationWhenSynchronizationIsPending)
{
	TestSynchronizable object;

	object.requestSynchronization();
	ASSERT_TRUE(object.needsSynchronization());

	object.synchronize();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 1);
}

TEST(SynchronizableTraitTest, SynchronizeCanBeCalledMultipleTimesButOnlyRunsWhenPending)
{
	TestSynchronizable object;

	object.requestSynchronization();
	object.synchronize();
	object.synchronize();
	object.synchronize();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 1);
}

TEST(SynchronizableTraitTest, RepeatedRequestSynchronizationBeforeSynchronizeOnlyRequiresOneSynchronizationPass)
{
	TestSynchronizable object;

	object.requestSynchronization();
	object.requestSynchronization();
	object.requestSynchronization();

	ASSERT_TRUE(object.needsSynchronization());

	object.synchronize();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 1);
}

TEST(SynchronizableTraitTest, RequestSynchronizationAfterSynchronizeAllowsAnotherSynchronizationPass)
{
	TestSynchronizable object;

	object.requestSynchronization();
	object.synchronize();

	ASSERT_FALSE(object.needsSynchronization());
	ASSERT_EQ(object.synchronizeCount(), 1);

	object.requestSynchronization();
	object.synchronize();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 2);
}

TEST(SynchronizableTraitTest, ForceSynchronizationCallsImplementationEvenWhenNotPending)
{
	TestSynchronizable object;

	object.forceSynchronization();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 1);
}

TEST(SynchronizableTraitTest, ForceSynchronizationClearsPendingSynchronizationFlag)
{
	TestSynchronizable object;

	object.requestSynchronization();
	ASSERT_TRUE(object.needsSynchronization());

	object.forceSynchronization();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 1);
}

TEST(SynchronizableTraitTest, ForceSynchronizationCanBeCalledMultipleTimes)
{
	TestSynchronizable object;

	object.forceSynchronization();
	object.forceSynchronization();
	object.forceSynchronization();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 3);
}

TEST(SynchronizableTraitTest, SynchronizeAfterForceWithoutNewRequestDoesNothing)
{
	TestSynchronizable object;

	object.forceSynchronization();
	ASSERT_EQ(object.synchronizeCount(), 1);
	ASSERT_FALSE(object.needsSynchronization());

	object.synchronize();

	EXPECT_EQ(object.synchronizeCount(), 1);
	EXPECT_FALSE(object.needsSynchronization());
}

TEST(SynchronizableTraitTest, RequestSynchronizationAfterForceAllowsDeferredSynchronizationAgain)
{
	TestSynchronizable object;

	object.forceSynchronization();
	ASSERT_EQ(object.synchronizeCount(), 1);
	ASSERT_FALSE(object.needsSynchronization());

	object.requestSynchronization();
	ASSERT_TRUE(object.needsSynchronization());

	object.synchronize();

	EXPECT_EQ(object.synchronizeCount(), 2);
	EXPECT_FALSE(object.needsSynchronization());
}

TEST(SynchronizableTraitTest, SynchronizationFlagTracksStateCorrectlyAcrossMultipleOperations)
{
	TestSynchronizable object;

	EXPECT_FALSE(object.needsSynchronization());

	object.requestSynchronization();
	EXPECT_TRUE(object.needsSynchronization());

	object.synchronize();
	EXPECT_FALSE(object.needsSynchronization());

	object.requestSynchronization();
	EXPECT_TRUE(object.needsSynchronization());

	object.forceSynchronization();
	EXPECT_FALSE(object.needsSynchronization());

	EXPECT_EQ(object.synchronizeCount(), 2);
}

TEST(SynchronizableTraitTest, MultipleRequestsAndForcesProduceExpectedSynchronizationCount)
{
	TestSynchronizable object;

	object.requestSynchronization();
	object.requestSynchronization();
	object.synchronize();

	object.requestSynchronization();
	object.forceSynchronization();

	object.forceSynchronization();

	object.requestSynchronization();
	object.synchronize();

	EXPECT_FALSE(object.needsSynchronization());
	EXPECT_EQ(object.synchronizeCount(), 4);
}