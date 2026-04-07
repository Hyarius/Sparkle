#include <gtest/gtest.h>

#include "spk_duration.hpp"
#include "spk_time_utils.hpp"
#include "spk_timestamp.hpp"

using namespace spk;

TEST(TimeUtils_NowNsMonoTest, SequentialCallsAreMonotonic)
{
	const std::uint64_t first = spk::TimeUtils::nowNsMono();
	const std::uint64_t second = spk::TimeUtils::nowNsMono();

	EXPECT_LE(first, second);
}

TEST(TimeUtils_GetTimeTest, SequentialCallsAreMonotonic)
{
	const spk::Timestamp first = spk::TimeUtils::getTime();
	const spk::Timestamp second = spk::TimeUtils::getTime();

	EXPECT_LE(first, second);
}

TEST(TimeUtils_GetTimeTest, ConsecutiveTimestampsProduceNonNegativeDuration)
{
	const spk::Timestamp first = spk::TimeUtils::getTime();
	const spk::Timestamp second = spk::TimeUtils::getTime();

	const spk::Duration delta = second - first;

	EXPECT_GE(delta.nanoseconds(), 0);
}

TEST(TimeUtils_SleepForTest, SleepForZeroDoesNotThrow)
{
	EXPECT_NO_THROW(spk::TimeUtils::sleepFor(spk::Duration()));
}

TEST(TimeUtils_SleepForTest, SleepForPositiveDurationDoesNotThrow)
{
	EXPECT_NO_THROW(spk::TimeUtils::sleepFor(1_ms));
}

TEST(TimeUtils_SleepForTest, SleepForAtLeastRequestedDurationWithinReasonableTolerance)
{
	const spk::Timestamp before = spk::TimeUtils::getTime();

	spk::TimeUtils::sleepFor(5_ms);

	const spk::Timestamp after = spk::TimeUtils::getTime();
	const spk::Duration elapsed = after - before;

	EXPECT_EQ(elapsed.milliseconds(), 5);
}

TEST(TimeUtils_SleepForTest, SleepForPreservesMonotonicity)
{
	const spk::Timestamp before = spk::TimeUtils::getTime();

	spk::TimeUtils::sleepFor(2_ms);

	const spk::Timestamp after = spk::TimeUtils::getTime();

	EXPECT_LE(before, after);
}