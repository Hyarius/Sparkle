#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "spk_blockable_trait.hpp"

namespace
{
	class TestBlockable : public spk::BlockableTrait
	{
	public:
		using Blocker = spk::BlockableTrait::Blocker;
		using Mode = spk::BlockableTrait::Mode;

	private:
		int _flushCount = 0;

	protected:
		void flushPending() override
		{
			++_flushCount;
		}

	public:
		TestBlockable() = default;
		~TestBlockable() = default;

		void markPending()
		{
			setPending();
		}

		int flushCount() const
		{
			return _flushCount;
		}
	};
}

TEST(BlockableTraitBlockerTest, DefaultConstructedBlockerIsInvalid)
{
	spk::BlockableTrait::Blocker blocker;

	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitTest, FreshObjectStartsFullyUnblocked)
{
	TestBlockable object;

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
	EXPECT_EQ(object.flushCount(), 0);
}

TEST(BlockableTraitTest, DefaultBlockModeIsIgnore)
{
	TestBlockable object;

	auto blocker = object.block();

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
	EXPECT_TRUE(blocker.isValid());
}

TEST(BlockableTraitTest, ExplicitIgnoreBlockBlocksOnlyIgnoreState)
{
	TestBlockable object;

	auto blocker = object.block(TestBlockable::Mode::Ignore);

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, ExplicitDelayBlockBlocksOnlyDelayState)
{
	TestBlockable object;

	auto blocker = object.block(TestBlockable::Mode::Delay);

	EXPECT_TRUE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, MultipleIgnoreBlocksStackCorrectly)
{
	TestBlockable object;

	auto first = object.block(TestBlockable::Mode::Ignore);
	auto second = object.block(TestBlockable::Mode::Ignore);
	auto third = object.block(TestBlockable::Mode::Ignore);

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());

	second.release();
	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());

	first.release();
	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());

	third.release();
	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, MultipleDelayBlocksStackCorrectly)
{
	TestBlockable object;

	auto first = object.block(TestBlockable::Mode::Delay);
	auto second = object.block(TestBlockable::Mode::Delay);
	auto third = object.block(TestBlockable::Mode::Delay);

	EXPECT_TRUE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	second.release();
	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	first.release();
	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	third.release();
	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, MixedIgnoreAndDelayBlocksAreTrackedSeparately)
{
	TestBlockable object;

	auto ignoreBlocker = object.block(TestBlockable::Mode::Ignore);
	auto delayBlocker = object.block(TestBlockable::Mode::Delay);

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	ignoreBlocker.release();

	EXPECT_TRUE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	delayBlocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, MoveConstructedBlockerKeepsOriginalObjectBlocked)
{
	TestBlockable object;

	auto source = object.block(TestBlockable::Mode::Delay);

	ASSERT_TRUE(object.isBlocked());
	ASSERT_TRUE(source.isValid());

	TestBlockable::Blocker destination(std::move(source));

	EXPECT_FALSE(source.isValid());
	EXPECT_TRUE(destination.isValid());
	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	destination.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, MoveAssignedBlockerTransfersBlockingOwnership)
{
	TestBlockable firstObject;
	TestBlockable secondObject;

	auto firstBlocker = firstObject.block(TestBlockable::Mode::Delay);
	auto secondBlocker = secondObject.block(TestBlockable::Mode::Ignore);

	ASSERT_TRUE(firstObject.isBlocked());
	ASSERT_TRUE(secondObject.isBlocked());

	secondBlocker = std::move(firstBlocker);

	EXPECT_FALSE(firstBlocker.isValid());
	EXPECT_TRUE(secondBlocker.isValid());

	EXPECT_TRUE(firstObject.isBlocked());
	EXPECT_TRUE(firstObject.isDelayBlocked());

	EXPECT_FALSE(secondObject.isBlocked());
	EXPECT_FALSE(secondObject.isIgnoreBlocked());

	secondBlocker.release();

	EXPECT_FALSE(firstObject.isBlocked());
	EXPECT_FALSE(secondObject.isBlocked());
}

TEST(BlockableTraitTest, TemporaryBlockerBlocksOnlyDuringItsLifetime)
{
	TestBlockable object;

	{
		auto blocker = object.block(TestBlockable::Mode::Ignore);
		EXPECT_TRUE(object.isBlocked());
		EXPECT_TRUE(object.isIgnoreBlocked());
	}

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, DefaultConstructedBlockerDoesNotAffectObject)
{
	TestBlockable object;
	TestBlockable::Blocker blocker;

	EXPECT_FALSE(blocker.isValid());
	EXPECT_FALSE(object.isBlocked());

	blocker.release();

	EXPECT_FALSE(blocker.isValid());
	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}

TEST(BlockableTraitTest, MarkPendingWithoutAnyBlockDoesNothing)
{
	TestBlockable object;

	object.markPending();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_EQ(object.flushCount(), 0);
}

TEST(BlockableTraitTest, MarkPendingDuringIgnoreBlockDoesNotFlushOnRelease)
{
	TestBlockable object;

	auto blocker = object.block(TestBlockable::Mode::Ignore);

	object.markPending();

	EXPECT_EQ(object.flushCount(), 0);

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_EQ(object.flushCount(), 0);
}

TEST(BlockableTraitTest, DelayBlockWithoutPendingDoesNotFlushOnRelease)
{
	TestBlockable object;

	auto blocker = object.block(TestBlockable::Mode::Delay);

	EXPECT_EQ(object.flushCount(), 0);

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_EQ(object.flushCount(), 0);
}

TEST(BlockableTraitTest, DelayBlockFlushesPendingWhenReleased)
{
	TestBlockable object;

	auto blocker = object.block(TestBlockable::Mode::Delay);

	object.markPending();

	EXPECT_EQ(object.flushCount(), 0);

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_EQ(object.flushCount(), 1);
}

TEST(BlockableTraitTest, MultiplePendingMarksDuringSameDelayPeriodFlushOnlyOnce)
{
	TestBlockable object;

	auto blocker = object.block(TestBlockable::Mode::Delay);

	object.markPending();
	object.markPending();
	object.markPending();

	EXPECT_EQ(object.flushCount(), 0);

	blocker.release();

	EXPECT_EQ(object.flushCount(), 1);
	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, NestedDelayBlocksFlushOnlyWhenLastDelayBlockIsReleased)
{
	TestBlockable object;

	auto first = object.block(TestBlockable::Mode::Delay);
	auto second = object.block(TestBlockable::Mode::Delay);

	object.markPending();

	first.release();

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isDelayBlocked());
	EXPECT_EQ(object.flushCount(), 0);

	second.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_EQ(object.flushCount(), 1);
}

TEST(BlockableTraitTest, MixedIgnoreAndDelayBlocksFlushOnlyAfterEverythingIsReleased)
{
	TestBlockable object;

	auto ignoreBlocker = object.block(TestBlockable::Mode::Ignore);
	auto delayBlocker = object.block(TestBlockable::Mode::Delay);

	object.markPending();

	delayBlocker.release();

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
	EXPECT_EQ(object.flushCount(), 0);

	ignoreBlocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_EQ(object.flushCount(), 1);
}

TEST(BlockableTraitTest, PendingStateIsClearedAfterFlush)
{
	TestBlockable object;

	{
		auto blocker = object.block(TestBlockable::Mode::Delay);
		object.markPending();
	}

	EXPECT_EQ(object.flushCount(), 1);

	{
		auto blocker = object.block(TestBlockable::Mode::Delay);
	}

	EXPECT_EQ(object.flushCount(), 1);
}

TEST(BlockableTraitTest, NewDelayPeriodCanFlushAgainAfterPreviousFlush)
{
	TestBlockable object;

	{
		auto blocker = object.block(TestBlockable::Mode::Delay);
		object.markPending();
	}

	EXPECT_EQ(object.flushCount(), 1);

	{
		auto blocker = object.block(TestBlockable::Mode::Delay);
		object.markPending();
	}

	EXPECT_EQ(object.flushCount(), 2);
}

TEST(BlockableTraitTest, MoveAssignedBlockerTransfersPendingDelayBlockWithoutFlushingIt)
{
	TestBlockable firstObject;
	TestBlockable secondObject;

	auto firstBlocker = firstObject.block(TestBlockable::Mode::Delay);
	auto secondBlocker = secondObject.block(TestBlockable::Mode::Ignore);

	firstObject.markPending();

	EXPECT_EQ(firstObject.flushCount(), 0);
	EXPECT_EQ(secondObject.flushCount(), 0);
	
	EXPECT_TRUE(firstObject.isBlocked());
	EXPECT_TRUE(secondObject.isBlocked());

	secondBlocker = std::move(firstBlocker);

	EXPECT_EQ(firstObject.flushCount(), 0);
	EXPECT_EQ(secondObject.flushCount(), 0);

	EXPECT_TRUE(firstObject.isBlocked());
	EXPECT_FALSE(secondObject.isBlocked());

	secondBlocker.release();

	EXPECT_FALSE(firstObject.isBlocked());
	EXPECT_EQ(firstObject.flushCount(), 1);
}

TEST(BlockableTraitTest, ReleasingInvalidBlockerIsSafe)
{
	TestBlockable::Blocker blocker;

	EXPECT_NO_THROW(blocker.release());
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitTest, ReleasingBlockerAfterOwnerDestructionIsSafe)
{
	TestBlockable::Blocker blocker;

	{
		auto object = std::make_unique<TestBlockable>();
		blocker = object->block(TestBlockable::Mode::Delay);
		object->markPending();

		ASSERT_TRUE(blocker.isValid());
		ASSERT_TRUE(object->isBlocked());
	}

	EXPECT_FALSE(blocker.isValid());
	EXPECT_NO_THROW(blocker.release());
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitTest, DelayAndIgnoreFlagsReturnToFalseAfterAllBlocksReleased)
{
	TestBlockable object;

	auto ignoreBlocker = object.block(TestBlockable::Mode::Ignore);
	auto delayBlockerA = object.block(TestBlockable::Mode::Delay);
	auto delayBlockerB = object.block(TestBlockable::Mode::Delay);

	ASSERT_TRUE(object.isBlocked());
	ASSERT_TRUE(object.isIgnoreBlocked());
	ASSERT_TRUE(object.isDelayBlocked());

	delayBlockerA.release();
	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(object.isIgnoreBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	ignoreBlocker.release();
	EXPECT_TRUE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_TRUE(object.isDelayBlocked());

	delayBlockerB.release();
	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(object.isIgnoreBlocked());
	EXPECT_FALSE(object.isDelayBlocked());
}