#include <string>

#include <gtest/gtest.h>

#include "spk_observable_value.hpp"

namespace
{
	struct CustomType
	{
		int value = 0;

		CustomType() = default;

		explicit CustomType(int p_value) :
			value(p_value)
		{
		}

		bool operator==(const CustomType& p_other) const
		{
			return value == p_other.value;
		}

		bool operator!=(const CustomType& p_other) const
		{
			return value != p_other.value;
		}

		CustomType& operator+=(int p_value)
		{
			value += p_value;
			return *this;
		}

		CustomType operator+(int p_value) const
		{
			return CustomType(value + p_value);
		}
	};
}

TEST(ObservableValueTest, DefaultConstructorInitializesDefaultValue)
{
	spk::ObservableValue<int> value;

	EXPECT_EQ(value.value(), 0);
	EXPECT_EQ(static_cast<const int&>(value), 0);
}

TEST(ObservableValueTest, CopyLikeConstructorStoresProvidedValue)
{
	spk::ObservableValue<int> value(42);

	EXPECT_EQ(value.value(), 42);
}

TEST(ObservableValueTest, MoveLikeConstructorStoresProvidedValue)
{
	std::string source = "hello";

	spk::ObservableValue<std::string> value(std::move(source));

	EXPECT_EQ(value.value(), "hello");
}

TEST(ObservableValueTest, ValueAccessorReturnsCurrentValue)
{
	spk::ObservableValue<int> value(7);

	const int& result = value.value();

	EXPECT_EQ(result, 7);
}

TEST(ObservableValueTest, ConversionOperatorReturnsCurrentValue)
{
	spk::ObservableValue<int> value(19);

	const int& converted = value;

	EXPECT_EQ(converted, 19);
}

TEST(ObservableValueTest, SetChangesValue)
{
	spk::ObservableValue<int> value(10);

	value.set(25);

	EXPECT_EQ(value.value(), 25);
}

TEST(ObservableValueTest, AssignmentOperatorChangesValue)
{
	spk::ObservableValue<int> value(10);

	value = 99;

	EXPECT_EQ(value.value(), 99);
}

TEST(ObservableValueTest, SetReturnsSelfReference)
{
	spk::ObservableValue<int> value(10);

	spk::ObservableValue<int>& result = value.set(30);

	EXPECT_EQ(&result, &value);
	EXPECT_EQ(value.value(), 30);
}

TEST(ObservableValueTest, AssignmentReturnsSelfReference)
{
	spk::ObservableValue<int> value(10);

	spk::ObservableValue<int>& result = (value = 44);

	EXPECT_EQ(&result, &value);
	EXPECT_EQ(value.value(), 44);
}

TEST(ObservableValueTest, NoNotificationWhenSetToSameValue)
{
	spk::ObservableValue<int> value(12);

	int notificationCount = 0;
	auto contract = value.subscribe([&notificationCount](const int&)
	{
		++notificationCount;
	});

	value.set(12);
	value = 12;

	EXPECT_EQ(notificationCount, 0);
}

TEST(ObservableValueTest, NotificationOccursWhenValueChangesUsingSet)
{
	spk::ObservableValue<int> value(5);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	value.set(15);

	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 15);
	EXPECT_EQ(value.value(), 15);
}

TEST(ObservableValueTest, NotificationOccursWhenValueChangesUsingAssignment)
{
	spk::ObservableValue<int> value(5);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	value = 18;

	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 18);
	EXPECT_EQ(value.value(), 18);
}

TEST(ObservableValueTest, AddOperatorReturnsComputedValue)
{
	spk::ObservableValue<int> value(10);

	auto result = value + 5;

	EXPECT_EQ(result, 15);
	EXPECT_EQ(value.value(), 10);
}

TEST(ObservableValueTest, SubtractOperatorReturnsComputedValue)
{
	spk::ObservableValue<int> value(10);

	auto result = value - 3;

	EXPECT_EQ(result, 7);
	EXPECT_EQ(value.value(), 10);
}

TEST(ObservableValueTest, MultiplyOperatorReturnsComputedValue)
{
	spk::ObservableValue<int> value(10);

	auto result = value * 4;

	EXPECT_EQ(result, 40);
	EXPECT_EQ(value.value(), 10);
}

TEST(ObservableValueTest, DivideOperatorReturnsComputedValue)
{
	spk::ObservableValue<int> value(20);

	auto result = value / 5;

	EXPECT_EQ(result, 4);
	EXPECT_EQ(value.value(), 20);
}

TEST(ObservableValueTest, AddAssignChangesValueAndNotifies)
{
	spk::ObservableValue<int> value(10);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	value += 6;

	EXPECT_EQ(value.value(), 16);
	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 16);
}

TEST(ObservableValueTest, SubtractAssignChangesValueAndNotifies)
{
	spk::ObservableValue<int> value(10);

	int notificationCount = 0;

	auto contract = value.subscribe([&notificationCount](const int&)
	{
		++notificationCount;
	});

	value -= 4;

	EXPECT_EQ(value.value(), 6);
	EXPECT_EQ(notificationCount, 1);
}

TEST(ObservableValueTest, MultiplyAssignChangesValueAndNotifies)
{
	spk::ObservableValue<int> value(3);

	int notificationCount = 0;

	auto contract = value.subscribe([&notificationCount](const int&)
	{
		++notificationCount;
	});

	value *= 7;

	EXPECT_EQ(value.value(), 21);
	EXPECT_EQ(notificationCount, 1);
}

TEST(ObservableValueTest, DivideAssignChangesValueAndNotifies)
{
	spk::ObservableValue<int> value(24);

	int notificationCount = 0;

	auto contract = value.subscribe([&notificationCount](const int&)
	{
		++notificationCount;
	});

	value /= 6;

	EXPECT_EQ(value.value(), 4);
	EXPECT_EQ(notificationCount, 1);
}

TEST(ObservableValueTest, EqualityOperatorsWork)
{
	spk::ObservableValue<int> value(10);

	EXPECT_TRUE(value == 10);
	EXPECT_FALSE(value == 9);

	EXPECT_TRUE(value != 9);
	EXPECT_FALSE(value != 10);
}

TEST(ObservableValueTest, OrderingOperatorsWork)
{
	spk::ObservableValue<int> value(10);

	EXPECT_TRUE(value < 11);
	EXPECT_FALSE(value < 10);

	EXPECT_TRUE(value <= 10);
	EXPECT_TRUE(value <= 11);
	EXPECT_FALSE(value <= 9);

	EXPECT_TRUE(value > 9);
	EXPECT_FALSE(value > 10);

	EXPECT_TRUE(value >= 10);
	EXPECT_TRUE(value >= 9);
	EXPECT_FALSE(value >= 11);
}

TEST(ObservableValueTest, PrefixIncrementChangesValueAndReturnsSelf)
{
	spk::ObservableValue<int> value(10);

	spk::ObservableValue<int>& result = ++value;

	EXPECT_EQ(&result, &value);
	EXPECT_EQ(value.value(), 11);
}

TEST(ObservableValueTest, PrefixIncrementNotifiesOnce)
{
	spk::ObservableValue<int> value(10);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	++value;

	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 11);
	EXPECT_EQ(value.value(), 11);
}

TEST(ObservableValueTest, PrefixDecrementChangesValueAndReturnsSelf)
{
	spk::ObservableValue<int> value(10);

	spk::ObservableValue<int>& result = --value;

	EXPECT_EQ(&result, &value);
	EXPECT_EQ(value.value(), 9);
}

TEST(ObservableValueTest, PrefixDecrementNotifiesOnce)
{
	spk::ObservableValue<int> value(10);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	--value;

	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 9);
	EXPECT_EQ(value.value(), 9);
}

TEST(ObservableValueTest, PostfixIncrementReturnsOldValueAndUpdatesStoredValue)
{
	spk::ObservableValue<int> value(10);

	int result = value++;

	EXPECT_EQ(result, 10);
	EXPECT_EQ(value.value(), 11);
}

TEST(ObservableValueTest, PostfixIncrementNotifiesOnce)
{
	spk::ObservableValue<int> value(10);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	int result = value++;

	EXPECT_EQ(result, 10);
	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 11);
	EXPECT_EQ(value.value(), 11);
}

TEST(ObservableValueTest, PostfixDecrementReturnsOldValueAndUpdatesStoredValue)
{
	spk::ObservableValue<int> value(10);

	int result = value--;

	EXPECT_EQ(result, 10);
	EXPECT_EQ(value.value(), 9);
}

TEST(ObservableValueTest, PostfixDecrementNotifiesOnce)
{
	spk::ObservableValue<int> value(10);

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const int& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	int result = value--;

	EXPECT_EQ(result, 10);
	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 9);
	EXPECT_EQ(value.value(), 9);
}

TEST(ObservableValueTest, StringSupportsSetAssignmentPlusAndPlusAssign)
{
	spk::ObservableValue<std::string> value("ab");

	int notificationCount = 0;
	std::string receivedValue;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const std::string& p_value)
	{
		++notificationCount;
		receivedValue = p_value;
	});

	EXPECT_EQ(value + std::string("cd"), "abcd");

	value += std::string("ef");
	EXPECT_EQ(value.value(), "abef");
	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, "abef");

	value = std::string("hello");
	EXPECT_EQ(value.value(), "hello");
	EXPECT_EQ(notificationCount, 2);
	EXPECT_EQ(receivedValue, "hello");
}

TEST(ObservableValueTest, CustomTypeSupportsEnabledOperationsOnly)
{
	spk::ObservableValue<CustomType> value(CustomType(10));

	int notificationCount = 0;
	int receivedValue = -1;

	auto contract = value.subscribe([&notificationCount, &receivedValue](const CustomType& p_value)
	{
		++notificationCount;
		receivedValue = p_value.value;
	});

	CustomType plusResult = value + 5;
	EXPECT_EQ(plusResult.value, 15);

	value += 7;

	EXPECT_EQ(value.value().value, 17);
	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(receivedValue, 17);
}

TEST(ObservableValueTest, MoveAssignmentPathUpdatesValue)
{
	spk::ObservableValue<std::string> value("start");

	std::string replacement = "moved";

	value = std::move(replacement);

	EXPECT_EQ(value.value(), "moved");
}

TEST(ObservableValueTest, RepeatedChangesNotifyForEachDistinctChange)
{
	spk::ObservableValue<int> value(1);

	int notificationCount = 0;
	int lastValue = -1;

	auto contract = value.subscribe([&notificationCount, &lastValue](const int& p_value)
	{
		++notificationCount;
		lastValue = p_value;
	});

	value = 2;
	value = 2;
	value = 3;
	value = 3;
	value = 4;

	EXPECT_EQ(notificationCount, 3);
	EXPECT_EQ(lastValue, 4);
	EXPECT_EQ(value.value(), 4);
}