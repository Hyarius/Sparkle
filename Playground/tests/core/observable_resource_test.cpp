#include "core/observable_resource.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(ObservableResource, ClampsCurrentAndMaximum)
{
	struct TestCase
	{
		int current;
		int max;
		int expectedCurrent;
		int expectedMax;
	};

	for (const TestCase &testCase : std::vector<TestCase>{
			 {5, 10, 5, 10},
			 {15, 10, 10, 10},
			 {-3, 10, 0, 10},
			 {5, -2, 0, 0}})
	{
		pg::ObservableResource resource;
		resource.set(testCase.current, testCase.max);
		EXPECT_EQ(resource.current(), testCase.expectedCurrent);
		EXPECT_EQ(resource.max(), testCase.expectedMax);
	}
}

TEST(ObservableResource, OverflowVariantOnlyClampsAtZero)
{
	pg::ObservableResource resource(5, 10);

	resource.setCurrentAllowOverflow(15);
	EXPECT_EQ(resource.current(), 15);
	EXPECT_EQ(resource.max(), 10);
	EXPECT_FLOAT_EQ(resource.ratio(), 1.5f);

	resource.setCurrentAllowOverflow(-4);
	EXPECT_EQ(resource.current(), 0);
}

TEST(ObservableResource, MaxChangeAndMutationsClampConsistently)
{
	pg::ObservableResource resource(8, 10);

	resource.setMax(5);
	EXPECT_EQ(resource.current(), 5);
	EXPECT_EQ(resource.max(), 5);

	resource.decrease(3);
	EXPECT_EQ(resource.current(), 2);
	resource.increase(10);
	EXPECT_EQ(resource.current(), 5);
	resource.change(-20);
	EXPECT_EQ(resource.current(), 0);
	resource.reset();
	EXPECT_EQ(resource.current(), 5);
}

TEST(ObservableResource, NotifiesOnlyOnChangeUnlessForced)
{
	pg::ObservableResource resource(5, 10);
	int notifications = 0;
	auto contract = resource.subscribe([&notifications](const pg::ObservableResource &) {
		++notifications;
	});

	resource.set(5, 10);
	EXPECT_EQ(notifications, 0);
	resource.setCurrent(6);
	EXPECT_EQ(notifications, 1);
	resource.set(6, 10, true);
	EXPECT_EQ(notifications, 2);
}

TEST(ObservableFloatResource, MirrorsIntegerSemantics)
{
	pg::ObservableFloatResource resource(0.5f, 1.0f);
	resource.increase(1.0f);
	EXPECT_FLOAT_EQ(resource.current(), 1.0f);
	resource.setCurrentAllowOverflow(1.25f);
	EXPECT_FLOAT_EQ(resource.ratio(), 1.25f);
	resource.setMax(-1.0f);
	EXPECT_FLOAT_EQ(resource.current(), 0.0f);
	EXPECT_FLOAT_EQ(resource.max(), 0.0f);
}

TEST(ObservableList, NotifiesForStructuralAndExplicitItemChanges)
{
	pg::ObservableList<std::string> list;
	int notifications = 0;
	auto contract = list.subscribe([&notifications](const pg::ObservableList<std::string> &) {
		++notifications;
	});

	list.add("first");
	EXPECT_EQ(notifications, 1);
	list[0] = "updated";
	list.notifyItemChangedAt(0);
	EXPECT_EQ(notifications, 2);
	list.add("second");
	list.removeAt(0);
	EXPECT_EQ(list.at(0), "second");
	EXPECT_EQ(notifications, 4);
	list.clear();
	EXPECT_EQ(notifications, 5);
	list.clear();
	EXPECT_EQ(notifications, 5);
}

TEST(ObservableList, RejectsInvalidNotificationIndex)
{
	pg::ObservableList<int> list;
	EXPECT_THROW(list.notifyItemChangedAt(0), std::out_of_range);
	EXPECT_THROW(list.removeAt(0), std::out_of_range);
}
