#include <gtest/gtest.h>

#include "structures/design_pattern/spk_priorizable_trait.hpp"

namespace
{
	class Prioritized : public spk::PriorizableTrait
	{
	public:
		Prioritized() = default;
		explicit Prioritized(Priority p_priority) :
			spk::PriorizableTrait(p_priority)
		{
		}
	};
}

TEST(PriorizableTraitTest, DefaultPriorityIsZero)
{
	Prioritized object;

	EXPECT_EQ(object.priority(), 0);
}

TEST(PriorizableTraitTest, ConstructorSetsInitialPriority)
{
	Prioritized object(42);

	EXPECT_EQ(object.priority(), 42);
}

TEST(PriorizableTraitTest, SetPriorityUpdatesValue)
{
	Prioritized object;

	object.setPriority(7);

	EXPECT_EQ(object.priority(), 7);
}

TEST(PriorizableTraitTest, SetPriorityNotifiesSubscribersOnChange)
{
	Prioritized object;
	int changeCount = 0;

	auto contract = object.subscribeToPriorityChange([&changeCount]() { ++changeCount; });

	object.setPriority(5);

	EXPECT_EQ(changeCount, 1);
}

TEST(PriorizableTraitTest, SetPriorityDoesNotNotifyWhenValueUnchanged)
{
	Prioritized object(3);
	int changeCount = 0;

	auto contract = object.subscribeToPriorityChange([&changeCount]() { ++changeCount; });

	object.setPriority(3);

	EXPECT_EQ(changeCount, 0);
}

TEST(PriorizableTraitTest, NegativePrioritiesAreSupported)
{
	Prioritized object;

	object.setPriority(-10);

	EXPECT_EQ(object.priority(), -10);
}
