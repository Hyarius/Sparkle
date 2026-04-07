#include <gtest/gtest.h>

#include <utility>

#include "spk_duration.hpp"
#include "spk_time_unit.hpp"
#include "spk_timestamp.hpp"

TEST(Timestamp_ConstructionTest, DefaultConstructorProducesMonotonicTimestamps)
{
	const spk::Timestamp first;
	const spk::Timestamp second;

	EXPECT_LE(first, second);
}

TEST(Timestamp_ConstructionTest, ConstructFromNanosecondsStoresCorrectValue)
{
	const spk::Timestamp timestamp(123456789.0L, spk::TimeUnit::Nanosecond);

	EXPECT_EQ(timestamp.nanoseconds(), 123456789LL);
	EXPECT_EQ(timestamp.milliseconds(), 123LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 0.123456789);
}

TEST(Timestamp_ConstructionTest, ConstructFromMillisecondsStoresCorrectValue)
{
	const spk::Timestamp timestamp(250.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(timestamp.nanoseconds(), 250000000LL);
	EXPECT_EQ(timestamp.milliseconds(), 250LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 0.25);
}

TEST(Timestamp_ConstructionTest, ConstructFromSecondsStoresCorrectValue)
{
	const spk::Timestamp timestamp(1.5L, spk::TimeUnit::Second);

	EXPECT_EQ(timestamp.nanoseconds(), 1500000000LL);
	EXPECT_EQ(timestamp.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.5);
}

TEST(Timestamp_ConstructionTest, ConstructFromDurationStoresCorrectValue)
{
	const spk::Duration duration(2.5L, spk::TimeUnit::Second);
	const spk::Timestamp timestamp(duration);

	EXPECT_EQ(timestamp.nanoseconds(), 2500000000LL);
	EXPECT_EQ(timestamp.milliseconds(), 2500LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 2.5);
}

TEST(Timestamp_ConstructionTest, CopyConstructorPreservesValue)
{
	const spk::Timestamp source(3.5L, spk::TimeUnit::Second);
	const spk::Timestamp copy(source);

	EXPECT_EQ(copy.nanoseconds(), source.nanoseconds());
	EXPECT_EQ(copy.milliseconds(), source.milliseconds());
	EXPECT_DOUBLE_EQ(copy.seconds(), source.seconds());
}

TEST(Timestamp_ConstructionTest, MoveConstructorPreservesValue)
{
	spk::Timestamp source(3.5L, spk::TimeUnit::Second);

	const long long sourceNanoseconds = source.nanoseconds();
	const long long sourceMilliseconds = source.milliseconds();
	const double sourceSeconds = source.seconds();

	const spk::Timestamp moved(std::move(source));

	EXPECT_EQ(moved.nanoseconds(), sourceNanoseconds);
	EXPECT_EQ(moved.milliseconds(), sourceMilliseconds);
	EXPECT_DOUBLE_EQ(moved.seconds(), sourceSeconds);
}

TEST(Timestamp_AssignmentTest, CopyAssignmentPreservesValue)
{
	const spk::Timestamp source(4.5L, spk::TimeUnit::Second);
	spk::Timestamp destination;

	destination = source;

	EXPECT_EQ(destination.nanoseconds(), source.nanoseconds());
	EXPECT_EQ(destination.milliseconds(), source.milliseconds());
	EXPECT_DOUBLE_EQ(destination.seconds(), source.seconds());
}

TEST(Timestamp_AssignmentTest, MoveAssignmentPreservesValue)
{
	spk::Timestamp source(4.5L, spk::TimeUnit::Second);

	const long long sourceNanoseconds = source.nanoseconds();
	const long long sourceMilliseconds = source.milliseconds();
	const double sourceSeconds = source.seconds();

	spk::Timestamp destination;
	destination = std::move(source);

	EXPECT_EQ(destination.nanoseconds(), sourceNanoseconds);
	EXPECT_EQ(destination.milliseconds(), sourceMilliseconds);
	EXPECT_DOUBLE_EQ(destination.seconds(), sourceSeconds);
}

TEST(Timestamp_ArithmeticTest, DifferenceBetweenTwoTimestampsReturnsDuration)
{
	const spk::Timestamp left(2.0L, spk::TimeUnit::Second);
	const spk::Timestamp right(500.0L, spk::TimeUnit::Millisecond);

	const spk::Duration result = left - right;

	EXPECT_EQ(result.nanoseconds(), 1500000000LL);
	EXPECT_EQ(result.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(result.seconds(), 1.5);
}

TEST(Timestamp_ArithmeticTest, AddingDurationToTimestampWorks)
{
	const spk::Timestamp timestamp(2.0L, spk::TimeUnit::Second);
	const spk::Duration duration(500.0L, spk::TimeUnit::Millisecond);

	const spk::Timestamp result = timestamp + duration;

	EXPECT_EQ(result.nanoseconds(), 2500000000LL);
	EXPECT_EQ(result.milliseconds(), 2500LL);
	EXPECT_DOUBLE_EQ(result.seconds(), 2.5);
}

TEST(Timestamp_ArithmeticTest, SubtractingDurationFromTimestampWorks)
{
	const spk::Timestamp timestamp(2.0L, spk::TimeUnit::Second);
	const spk::Duration duration(500.0L, spk::TimeUnit::Millisecond);

	const spk::Timestamp result = timestamp - duration;

	EXPECT_EQ(result.nanoseconds(), 1500000000LL);
	EXPECT_EQ(result.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(result.seconds(), 1.5);
}

TEST(Timestamp_ArithmeticTest, PlusEqualsWorks)
{
	spk::Timestamp timestamp(2.0L, spk::TimeUnit::Second);
	timestamp += spk::Duration(500.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(timestamp.nanoseconds(), 2500000000LL);
	EXPECT_EQ(timestamp.milliseconds(), 2500LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 2.5);
}

TEST(Timestamp_ArithmeticTest, MinusEqualsWorks)
{
	spk::Timestamp timestamp(2.0L, spk::TimeUnit::Second);
	timestamp -= spk::Duration(500.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(timestamp.nanoseconds(), 1500000000LL);
	EXPECT_EQ(timestamp.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.5);
}

TEST(Timestamp_ComparisonTest, EqualityWorksAcrossUnits)
{
	const spk::Timestamp left(1000.0L, spk::TimeUnit::Millisecond);
	const spk::Timestamp right(1.0L, spk::TimeUnit::Second);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);
}

TEST(Timestamp_ComparisonTest, OrderingWorks)
{
	const spk::Timestamp earlier(999.0L, spk::TimeUnit::Millisecond);
	const spk::Timestamp later(1.0L, spk::TimeUnit::Second);

	EXPECT_TRUE(earlier < later);
	EXPECT_TRUE(earlier <= later);
	EXPECT_FALSE(earlier > later);
	EXPECT_FALSE(earlier >= later);

	EXPECT_TRUE(later > earlier);
	EXPECT_TRUE(later >= earlier);
	EXPECT_FALSE(later < earlier);
	EXPECT_FALSE(later <= earlier);
}

TEST(Timestamp_CacheBehaviorTest, RepeatedMillisecondsCallsReturnConsistentValue)
{
	const spk::Timestamp timestamp(1234.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(timestamp.milliseconds(), 1234LL);
	EXPECT_EQ(timestamp.milliseconds(), 1234LL);
	EXPECT_EQ(timestamp.milliseconds(), 1234LL);
}

TEST(Timestamp_CacheBehaviorTest, RepeatedSecondsCallsReturnConsistentValue)
{
	const spk::Timestamp timestamp(1.25L, spk::TimeUnit::Second);

	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.25);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.25);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.25);
}

TEST(Timestamp_CacheBehaviorTest, CacheStaysCorrectAfterMutation)
{
	spk::Timestamp timestamp(1.0L, spk::TimeUnit::Second);

	EXPECT_EQ(timestamp.milliseconds(), 1000LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.0);

	timestamp += spk::Duration(500.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(timestamp.nanoseconds(), 1500000000LL);
	EXPECT_EQ(timestamp.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(timestamp.seconds(), 1.5);
}