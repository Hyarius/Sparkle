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
	};
}

TEST(BlockableTraitBlockerTest, DefaultConstructedBlockerIsInvalid)
{
	spk::BlockableTrait::Blocker blocker;

	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitBlockerTest, ConstructingBlockerFromCounterIncrementsCounter)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	{
		spk::BlockableTrait::Blocker blocker(counter);

		ASSERT_NE(counter, nullptr);
		EXPECT_EQ(*counter, 1u);
		EXPECT_TRUE(blocker.isValid());
	}

	EXPECT_EQ(*counter, 0u);
}

TEST(BlockableTraitBlockerTest, MultipleBlockersStackOnSameCounter)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	{
		spk::BlockableTrait::Blocker first(counter);
		EXPECT_EQ(*counter, 1u);

		{
			spk::BlockableTrait::Blocker second(counter);
			EXPECT_EQ(*counter, 2u);

			{
				spk::BlockableTrait::Blocker third(counter);
				EXPECT_EQ(*counter, 3u);
			}

			EXPECT_EQ(*counter, 2u);
		}

		EXPECT_EQ(*counter, 1u);
	}

	EXPECT_EQ(*counter, 0u);
}

TEST(BlockableTraitBlockerTest, ReleaseDecrementsCounterAndInvalidatesBlocker)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	spk::BlockableTrait::Blocker blocker(counter);

	ASSERT_TRUE(blocker.isValid());
	ASSERT_EQ(*counter, 1u);

	blocker.release();

	EXPECT_EQ(*counter, 0u);
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitBlockerTest, ReleasingTwiceIsSafeAndDoesNotUnderflowCounter)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	spk::BlockableTrait::Blocker blocker(counter);

	ASSERT_EQ(*counter, 1u);

	blocker.release();
	EXPECT_EQ(*counter, 0u);
	EXPECT_FALSE(blocker.isValid());

	blocker.release();
	EXPECT_EQ(*counter, 0u);
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitBlockerTest, DestructorReleasesAutomatically)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	{
		spk::BlockableTrait::Blocker blocker(counter);
		EXPECT_EQ(*counter, 1u);
	}

	EXPECT_EQ(*counter, 0u);
}

TEST(BlockableTraitBlockerTest, MoveConstructionTransfersOwnership)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	spk::BlockableTrait::Blocker source(counter);

	ASSERT_EQ(*counter, 1u);
	ASSERT_TRUE(source.isValid());

	spk::BlockableTrait::Blocker destination(std::move(source));

	EXPECT_EQ(*counter, 1u);
	EXPECT_FALSE(source.isValid());
	EXPECT_TRUE(destination.isValid());

	destination.release();

	EXPECT_EQ(*counter, 0u);
	EXPECT_FALSE(destination.isValid());
}

TEST(BlockableTraitBlockerTest, MoveAssignmentTransfersOwnershipAndReleasesPreviousCounter)
{
	std::shared_ptr<size_t> firstCounter = std::make_shared<size_t>(0);
	std::shared_ptr<size_t> secondCounter = std::make_shared<size_t>(0);

	spk::BlockableTrait::Blocker first(firstCounter);
	spk::BlockableTrait::Blocker second(secondCounter);

	ASSERT_EQ(*firstCounter, 1u);
	ASSERT_EQ(*secondCounter, 1u);

	second = std::move(first);

	EXPECT_EQ(*firstCounter, 1u);
	EXPECT_EQ(*secondCounter, 0u);

	EXPECT_FALSE(first.isValid());
	EXPECT_TRUE(second.isValid());

	second.release();

	EXPECT_EQ(*firstCounter, 0u);
	EXPECT_EQ(*secondCounter, 0u);
}

TEST(BlockableTraitBlockerTest, SelfMoveAssignmentDoesNotBreakBlocker)
{
	std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);

	spk::BlockableTrait::Blocker blocker(counter);

	ASSERT_EQ(*counter, 1u);
	ASSERT_TRUE(blocker.isValid());

	blocker = std::move(blocker);

	EXPECT_EQ(*counter, 1u);
	EXPECT_TRUE(blocker.isValid());

	blocker.release();

	EXPECT_EQ(*counter, 0u);
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitBlockerTest, ReleaseIsSafeWhenCounterExpired)
{
	spk::BlockableTrait::Blocker blocker;

	{
		std::shared_ptr<size_t> counter = std::make_shared<size_t>(0);
		blocker = spk::BlockableTrait::Blocker(counter);

		ASSERT_TRUE(blocker.isValid());
		ASSERT_EQ(*counter, 1u);
	}

	EXPECT_FALSE(blocker.isValid());

	EXPECT_NO_THROW(blocker.release());
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitTest, FreshObjectIsNotBlocked)
{
	TestBlockable object;

	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, SingleBlockBlocksObjectUntilBlockerReleased)
{
	TestBlockable object;

	TestBlockable::Blocker blocker = object.block();

	EXPECT_TRUE(object.isBlocked());
	EXPECT_TRUE(blocker.isValid());

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(blocker.isValid());
}

TEST(BlockableTraitTest, BlockStateStacksWithMultipleBlockers)
{
	TestBlockable object;

	TestBlockable::Blocker first = object.block();
	EXPECT_TRUE(object.isBlocked());

	TestBlockable::Blocker second = object.block();
	EXPECT_TRUE(object.isBlocked());

	first.release();
	EXPECT_TRUE(object.isBlocked());

	second.release();
	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, MoveConstructedBlockerStillBlocksOriginalObject)
{
	TestBlockable object;

	TestBlockable::Blocker first = object.block();

	ASSERT_TRUE(object.isBlocked());
	ASSERT_TRUE(first.isValid());

	TestBlockable::Blocker second(std::move(first));

	EXPECT_FALSE(first.isValid());
	EXPECT_TRUE(second.isValid());
	EXPECT_TRUE(object.isBlocked());

	second.release();

	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, MoveAssignedBlockerStillBlocksOriginalObject)
{
	TestBlockable object;
	TestBlockable otherObject;

	TestBlockable::Blocker blockerOnFirst = object.block();
	TestBlockable::Blocker blockerOnSecond = otherObject.block();

	ASSERT_TRUE(object.isBlocked());
	ASSERT_TRUE(otherObject.isBlocked());

	blockerOnSecond = std::move(blockerOnFirst);

	EXPECT_FALSE(blockerOnFirst.isValid());
	EXPECT_TRUE(blockerOnSecond.isValid());

	EXPECT_TRUE(object.isBlocked());
	EXPECT_FALSE(otherObject.isBlocked());

	blockerOnSecond.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(otherObject.isBlocked());
}

TEST(BlockableTraitTest, TemporaryBlockerOnlyBlocksForScopeDuration)
{
	TestBlockable object;

	{
		TestBlockable::Blocker blocker = object.block();
		EXPECT_TRUE(object.isBlocked());
	}

	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, MultipleTemporaryBlockersUnblockInNaturalScopeOrder)
{
	TestBlockable object;

	{
		TestBlockable::Blocker first = object.block();
		EXPECT_TRUE(object.isBlocked());

		{
			TestBlockable::Blocker second = object.block();
			EXPECT_TRUE(object.isBlocked());
		}

		EXPECT_TRUE(object.isBlocked());
	}

	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, ReleasingOneOfSeveralBlockersKeepsObjectBlocked)
{
	TestBlockable object;

	TestBlockable::Blocker first = object.block();
	TestBlockable::Blocker second = object.block();
	TestBlockable::Blocker third = object.block();

	ASSERT_TRUE(object.isBlocked());

	second.release();
	EXPECT_TRUE(object.isBlocked());

	first.release();
	EXPECT_TRUE(object.isBlocked());

	third.release();
	EXPECT_FALSE(object.isBlocked());
}

TEST(BlockableTraitTest, DefaultConstructedBlockerDoesNotAffectObject)
{
	TestBlockable object;
	TestBlockable::Blocker blocker;

	EXPECT_FALSE(blocker.isValid());
	EXPECT_FALSE(object.isBlocked());

	blocker.release();

	EXPECT_FALSE(object.isBlocked());
	EXPECT_FALSE(blocker.isValid());
}