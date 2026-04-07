#include <gtest/gtest.h>

#include <utility>

#include "spk_duration.hpp"
#include "spk_time_unit.hpp"

using namespace spk;

TEST(Duration_ConstructionTest, DefaultConstructorBuildsZeroDuration)
{
	const spk::Duration duration;

	EXPECT_EQ(duration.nanoseconds(), 0);
	EXPECT_EQ(duration.milliseconds(), 0);
	EXPECT_DOUBLE_EQ(duration.seconds(), 0.0);
}

TEST(Duration_ConstructionTest, ConstructFromNanosecondsStoresCorrectValue)
{
	const spk::Duration duration(123456789.0L, spk::TimeUnit::Nanosecond);

	EXPECT_EQ(duration.nanoseconds(), 123456789LL);
	EXPECT_EQ(duration.milliseconds(), 123LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 0.123456789);
}

TEST(Duration_ConstructionTest, ConstructFromMillisecondsStoresCorrectValue)
{
	const spk::Duration duration(250.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(duration.nanoseconds(), 250000000LL);
	EXPECT_EQ(duration.milliseconds(), 250LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 0.25);
}

TEST(Duration_ConstructionTest, ConstructFromSecondsStoresCorrectValue)
{
	const spk::Duration duration(1.5L, spk::TimeUnit::Second);

	EXPECT_EQ(duration.nanoseconds(), 1500000000LL);
	EXPECT_EQ(duration.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 1.5);
}

TEST(Duration_ConstructionTest, CopyConstructorPreservesValue)
{
	const spk::Duration source(2.5L, spk::TimeUnit::Second);
	const spk::Duration copy(source);

	EXPECT_EQ(copy.nanoseconds(), source.nanoseconds());
	EXPECT_EQ(copy.milliseconds(), source.milliseconds());
	EXPECT_DOUBLE_EQ(copy.seconds(), source.seconds());
}

TEST(Duration_ConstructionTest, MoveConstructorPreservesValue)
{
	spk::Duration source(2.5L, spk::TimeUnit::Second);

	const long long sourceNanoseconds = source.nanoseconds();
	const long long sourceMilliseconds = source.milliseconds();
	const double sourceSeconds = source.seconds();

	const spk::Duration moved(std::move(source));

	EXPECT_EQ(moved.nanoseconds(), sourceNanoseconds);
	EXPECT_EQ(moved.milliseconds(), sourceMilliseconds);
	EXPECT_DOUBLE_EQ(moved.seconds(), sourceSeconds);
}

TEST(Duration_AssignmentTest, CopyAssignmentPreservesValue)
{
	const spk::Duration source(3.25L, spk::TimeUnit::Second);
	spk::Duration destination;

	destination = source;

	EXPECT_EQ(destination.nanoseconds(), source.nanoseconds());
	EXPECT_EQ(destination.milliseconds(), source.milliseconds());
	EXPECT_DOUBLE_EQ(destination.seconds(), source.seconds());
}

TEST(Duration_AssignmentTest, MoveAssignmentPreservesValue)
{
	spk::Duration source(3.25L, spk::TimeUnit::Second);

	const long long sourceNanoseconds = source.nanoseconds();
	const long long sourceMilliseconds = source.milliseconds();
	const double sourceSeconds = source.seconds();

	spk::Duration destination;
	destination = std::move(source);

	EXPECT_EQ(destination.nanoseconds(), sourceNanoseconds);
	EXPECT_EQ(destination.milliseconds(), sourceMilliseconds);
	EXPECT_DOUBLE_EQ(destination.seconds(), sourceSeconds);
}

TEST(Duration_LiteralTest, IntegerSecondsLiteralWorks)
{
	const spk::Duration duration = 2_s;

	EXPECT_EQ(duration.nanoseconds(), 2000000000LL);
	EXPECT_EQ(duration.milliseconds(), 2000LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 2.0);
}

TEST(Duration_LiteralTest, FloatingSecondsLiteralWorks)
{
	const spk::Duration duration = 2.5_s;

	EXPECT_EQ(duration.nanoseconds(), 2500000000LL);
	EXPECT_EQ(duration.milliseconds(), 2500LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 2.5);
}

TEST(Duration_LiteralTest, MillisecondsLiteralWorks)
{
	const spk::Duration duration = 250_ms;

	EXPECT_EQ(duration.nanoseconds(), 250000000LL);
	EXPECT_EQ(duration.milliseconds(), 250LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 0.25);
}

TEST(Duration_LiteralTest, NanosecondsLiteralWorks)
{
	const spk::Duration duration = 500_ns;

	EXPECT_EQ(duration.nanoseconds(), 500LL);
	EXPECT_EQ(duration.milliseconds(), 0LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 0.0000005);
}

TEST(Duration_UnaryTest, UnaryMinusNegatesValue)
{
	const spk::Duration duration(250.0L, spk::TimeUnit::Millisecond);
	const spk::Duration negated = -duration;

	EXPECT_EQ(negated.nanoseconds(), -250000000LL);
	EXPECT_EQ(negated.milliseconds(), -250LL);
	EXPECT_DOUBLE_EQ(negated.seconds(), -0.25);
}

TEST(Duration_ArithmeticTest, AdditionWorks)
{
	const spk::Duration left(1.5L, spk::TimeUnit::Second);
	const spk::Duration right(250.0L, spk::TimeUnit::Millisecond);

	const spk::Duration result = left + right;

	EXPECT_EQ(result.nanoseconds(), 1750000000LL);
	EXPECT_EQ(result.milliseconds(), 1750LL);
	EXPECT_DOUBLE_EQ(result.seconds(), 1.75);
}

TEST(Duration_ArithmeticTest, SubtractionWorks)
{
	const spk::Duration left(1.5L, spk::TimeUnit::Second);
	const spk::Duration right(250.0L, spk::TimeUnit::Millisecond);

	const spk::Duration result = left - right;

	EXPECT_EQ(result.nanoseconds(), 1250000000LL);
	EXPECT_EQ(result.milliseconds(), 1250LL);
	EXPECT_DOUBLE_EQ(result.seconds(), 1.25);
}

TEST(Duration_ArithmeticTest, PlusEqualsWorks)
{
	spk::Duration value(1.5L, spk::TimeUnit::Second);
	value += spk::Duration(250.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(value.nanoseconds(), 1750000000LL);
	EXPECT_EQ(value.milliseconds(), 1750LL);
	EXPECT_DOUBLE_EQ(value.seconds(), 1.75);
}

TEST(Duration_ArithmeticTest, MinusEqualsWorks)
{
	spk::Duration value(1.5L, spk::TimeUnit::Second);
	value -= spk::Duration(250.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(value.nanoseconds(), 1250000000LL);
	EXPECT_EQ(value.milliseconds(), 1250LL);
	EXPECT_DOUBLE_EQ(value.seconds(), 1.25);
}

TEST(Duration_ComparisonTest, EqualityWorksAcrossUnits)
{
	const spk::Duration left(1000.0L, spk::TimeUnit::Millisecond);
	const spk::Duration right(1.0L, spk::TimeUnit::Second);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);
}

TEST(Duration_ComparisonTest, OrderingWorks)
{
	const spk::Duration shorter(999.0L, spk::TimeUnit::Millisecond);
	const spk::Duration longer(1.0L, spk::TimeUnit::Second);

	EXPECT_TRUE(shorter < longer);
	EXPECT_TRUE(shorter <= longer);
	EXPECT_FALSE(shorter > longer);
	EXPECT_FALSE(shorter >= longer);

	EXPECT_TRUE(longer > shorter);
	EXPECT_TRUE(longer >= shorter);
	EXPECT_FALSE(longer < shorter);
	EXPECT_FALSE(longer <= shorter);
}

TEST(Duration_CacheBehaviorTest, RepeatedMillisecondsCallsReturnConsistentValue)
{
	const spk::Duration duration(1234.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(duration.milliseconds(), 1234LL);
	EXPECT_EQ(duration.milliseconds(), 1234LL);
	EXPECT_EQ(duration.milliseconds(), 1234LL);
}

TEST(Duration_CacheBehaviorTest, RepeatedSecondsCallsReturnConsistentValue)
{
	const spk::Duration duration(1.25L, spk::TimeUnit::Second);

	EXPECT_DOUBLE_EQ(duration.seconds(), 1.25);
	EXPECT_DOUBLE_EQ(duration.seconds(), 1.25);
	EXPECT_DOUBLE_EQ(duration.seconds(), 1.25);
}

TEST(Duration_CacheBehaviorTest, CacheStaysCorrectAfterMutation)
{
	spk::Duration duration(1.0L, spk::TimeUnit::Second);

	EXPECT_EQ(duration.milliseconds(), 1000LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 1.0);

	duration += spk::Duration(500.0L, spk::TimeUnit::Millisecond);

	EXPECT_EQ(duration.nanoseconds(), 1500000000LL);
	EXPECT_EQ(duration.milliseconds(), 1500LL);
	EXPECT_DOUBLE_EQ(duration.seconds(), 1.5);
}