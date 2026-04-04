#include <gtest/gtest.h>

#include "spk_activable_trait.hpp"

namespace
{
	class TestActivableObject : public spk::ActivableTrait
	{
	public:
		TestActivableObject() = default;
		~TestActivableObject() = default;
	};
}

TEST(ActivableTraitTest, NewObjectStartsDeactivated)
{
	TestActivableObject object;

	EXPECT_FALSE(object.isActivated());
}

TEST(ActivableTraitTest, ActivateChangesStateToActivated)
{
	TestActivableObject object;

	object.activate();

	EXPECT_TRUE(object.isActivated());
}

TEST(ActivableTraitTest, DeactivateKeepsStateDeactivatedWhenAlreadyInactive)
{
	TestActivableObject object;

	object.deactivate();

	EXPECT_FALSE(object.isActivated());
}

TEST(ActivableTraitTest, DeactivateChangesStateToDeactivatedAfterActivation)
{
	TestActivableObject object;

	object.activate();
	ASSERT_TRUE(object.isActivated());

	object.deactivate();

	EXPECT_FALSE(object.isActivated());
}

TEST(ActivableTraitTest, ActivationSubscriptionIsTriggeredOnActivation)
{
	TestActivableObject object;
	int activationCount = 0;

	auto contract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	ASSERT_TRUE(contract.isValid());

	object.activate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(activationCount, 1);
}

TEST(ActivableTraitTest, DeactivationSubscriptionIsTriggeredOnDeactivation)
{
	TestActivableObject object;
	int deactivationCount = 0;

	auto contract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	ASSERT_TRUE(contract.isValid());

	object.activate();
	object.deactivate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, ActivationDoesNotTriggerActivationTwiceWhenAlreadyActivated)
{
	TestActivableObject object;
	int activationCount = 0;

	auto contract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	object.activate();
	object.activate();
	object.activate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(activationCount, 1);
}

TEST(ActivableTraitTest, DeactivationDoesNotTriggerDeactivationTwiceWhenAlreadyDeactivated)
{
	TestActivableObject object;
	int deactivationCount = 0;

	auto contract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.deactivate();
	object.deactivate();
	object.deactivate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(deactivationCount, 0);
}

TEST(ActivableTraitTest, DeactivationDoesNotTriggerTwiceAfterReturningToInactiveState)
{
	TestActivableObject object;
	int deactivationCount = 0;

	auto contract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.activate();
	object.deactivate();
	object.deactivate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, ActivationAndDeactivationUseSeparateProviders)
{
	TestActivableObject object;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	auto deactivationContract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.activate();

	EXPECT_EQ(activationCount, 1);
	EXPECT_EQ(deactivationCount, 0);

	object.deactivate();

	EXPECT_EQ(activationCount, 1);
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, ActivationCanBeTriggeredAgainAfterDeactivation)
{
	TestActivableObject object;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	auto deactivationContract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.activate();
	object.deactivate();
	object.activate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(activationCount, 2);
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, BlockedObjectCannotActivate)
{
	TestActivableObject object;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	auto deactivationContract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	auto blocker = object.block();

	object.activate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(activationCount, 0);
	EXPECT_EQ(deactivationCount, 0);
}

TEST(ActivableTraitTest, BlockedObjectCannotDeactivate)
{
	TestActivableObject object;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	auto deactivationContract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.activate();
	ASSERT_TRUE(object.isActivated());
	ASSERT_EQ(activationCount, 1);

	auto blocker = object.block();

	object.deactivate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(activationCount, 1);
	EXPECT_EQ(deactivationCount, 0);
}

TEST(ActivableTraitTest, ObjectCanActivateAfterBlockIsReleased)
{
	TestActivableObject object;
	int activationCount = 0;

	auto contract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	auto blocker = object.block();

	object.activate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(activationCount, 0);

	blocker.release();

	object.activate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(activationCount, 1);
}

TEST(ActivableTraitTest, ObjectCanDeactivateAfterBlockIsReleased)
{
	TestActivableObject object;
	int deactivationCount = 0;

	auto contract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.activate();
	ASSERT_TRUE(object.isActivated());

	auto blocker = object.block();

	object.deactivate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(deactivationCount, 0);

	blocker.release();

	object.deactivate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, ActivationSubscriptionCanBeResigned)
{
	TestActivableObject object;
	int activationCount = 0;

	auto contract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	contract.resign();

	object.activate();

	EXPECT_TRUE(object.isActivated());
	EXPECT_EQ(activationCount, 0);
}

TEST(ActivableTraitTest, DeactivationSubscriptionCanBeResigned)
{
	TestActivableObject object;
	int deactivationCount = 0;

	auto contract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	object.activate();
	contract.resign();
	object.deactivate();

	EXPECT_FALSE(object.isActivated());
	EXPECT_EQ(deactivationCount, 0);
}

TEST(ActivableTraitTest, ActivationAndDeactivationContractsAreIndependent)
{
	TestActivableObject object;
	int activationCount = 0;
	int deactivationCount = 0;

	auto activationContract = object.subscribeToActivation([&activationCount]()
	{
		++activationCount;
	});

	auto deactivationContract = object.subscribeToDeactivation([&deactivationCount]()
	{
		++deactivationCount;
	});

	activationContract.resign();

	object.activate();
	object.deactivate();

	EXPECT_EQ(activationCount, 0);
	EXPECT_EQ(deactivationCount, 1);
}

TEST(ActivableTraitTest, MultipleActivationSubscribersAreAllTriggered)
{
	TestActivableObject object;
	int firstCount = 0;
	int secondCount = 0;
	int thirdCount = 0;

	auto firstContract = object.subscribeToActivation([&firstCount]()
	{
		++firstCount;
	});

	auto secondContract = object.subscribeToActivation([&secondCount]()
	{
		++secondCount;
	});

	auto thirdContract = object.subscribeToActivation([&thirdCount]()
	{
		++thirdCount;
	});

	object.activate();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
	EXPECT_EQ(thirdCount, 1);
}

TEST(ActivableTraitTest, MultipleDeactivationSubscribersAreAllTriggered)
{
	TestActivableObject object;
	int firstCount = 0;
	int secondCount = 0;
	int thirdCount = 0;

	auto firstContract = object.subscribeToDeactivation([&firstCount]()
	{
		++firstCount;
	});

	auto secondContract = object.subscribeToDeactivation([&secondCount]()
	{
		++secondCount;
	});

	auto thirdContract = object.subscribeToDeactivation([&thirdCount]()
	{
		++thirdCount;
	});

	object.activate();
	object.deactivate();

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
	EXPECT_EQ(thirdCount, 1);
}