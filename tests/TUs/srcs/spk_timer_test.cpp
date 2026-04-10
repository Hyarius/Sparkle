#include <gtest/gtest.h>

#include <chrono>
#include <sstream>
#include <thread>

#include "spk_timer.hpp"

#include "spk_time_utils.hpp"

using namespace spk;

namespace
{
	void expectDurationAtLeast(const spk::Duration& p_duration, long long p_minimumNanoseconds)
	{
		EXPECT_GE(p_duration.nanoseconds(), p_minimumNanoseconds);
	}

	void expectDurationAtMost(const spk::Duration& p_duration, long long p_maximumNanoseconds)
	{
		EXPECT_LE(p_duration.nanoseconds(), p_maximumNanoseconds);
	}

	void expectDurationBetween(const spk::Duration& p_duration, long long p_minimumNanoseconds, long long p_maximumNanoseconds)
	{
		expectDurationAtLeast(p_duration, p_minimumNanoseconds);
		expectDurationAtMost(p_duration, p_maximumNanoseconds);
	}
}

TEST(TimerTest, DefaultConstructionStartsIdleWithZeroElapsedAndZeroExpectedDuration)
{
	spk::Timer timer;

	EXPECT_EQ(timer.state(), spk::Timer::State::Idle);
	EXPECT_EQ(timer.elapsed().nanoseconds(), 0LL);
	EXPECT_EQ(timer.expectedDuration().nanoseconds(), 0LL);
	EXPECT_FALSE(timer.hasTimedOut());
	EXPECT_FLOAT_EQ(timer.elapsedRatio(), 0.0f);
}

TEST(TimerTest, ConstructionWithExpectedDurationStoresExpectedDuration)
{
	const spk::Duration expectedDuration = 50_ms;
	spk::Timer timer(expectedDuration);

	EXPECT_EQ(timer.state(), spk::Timer::State::Idle);
	EXPECT_EQ(timer.expectedDuration().nanoseconds(), expectedDuration.nanoseconds());
	EXPECT_EQ(timer.elapsed().nanoseconds(), 0LL);
}

TEST(TimerTest, StartTransitionsToRunningAndElapsedGrows)
{
	spk::Timer timer(200_ms);

	timer.start();

	EXPECT_EQ(timer.state(), spk::Timer::State::Running);

	spk::TimeUtils::sleepFor(spk::Duration(30_ms));

	const spk::Duration elapsedTime = timer.elapsed();

	expectDurationAtLeast(elapsedTime, 10'000'000LL);
	EXPECT_EQ(timer.state(), spk::Timer::State::Running);
	EXPECT_FALSE(timer.hasTimedOut());
}

TEST(TimerTest, StopResetsStateAndElapsedTime)
{
	spk::Timer timer(200_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(25_ms));

	timer.stop();

	EXPECT_EQ(timer.state(), spk::Timer::State::Idle);
	EXPECT_EQ(timer.elapsed().nanoseconds(), 0LL);
	EXPECT_FALSE(timer.hasTimedOut());
	EXPECT_FLOAT_EQ(timer.elapsedRatio(), 0.0f);
}

TEST(TimerTest, PauseFreezesElapsedTime)
{
	spk::Timer timer(200_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(30_ms));

	timer.pause();

	EXPECT_EQ(timer.state(), spk::Timer::State::Paused);

	const spk::Duration elapsedTimeBeforeWait = timer.elapsed();

	spk::TimeUtils::sleepFor(spk::Duration(30_ms));

	const spk::Duration elapsedTimeAfterWait = timer.elapsed();

	expectDurationBetween(elapsedTimeBeforeWait, 10'000'000LL, 200'000'000LL);
	EXPECT_EQ(elapsedTimeAfterWait.nanoseconds(), elapsedTimeBeforeWait.nanoseconds());
	EXPECT_FALSE(timer.hasTimedOut());
}

TEST(TimerTest, ResumeContinuesAccumulatingTimeAfterPause)
{
	spk::Timer timer(300_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));
	timer.pause();

	const spk::Duration elapsedTimeWhilePaused = timer.elapsed();

	spk::TimeUtils::sleepFor(spk::Duration(20_ms));
	EXPECT_EQ(timer.elapsed().nanoseconds(), elapsedTimeWhilePaused.nanoseconds());

	timer.resume();

	EXPECT_EQ(timer.state(), spk::Timer::State::Running);

	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	const spk::Duration elapsedTimeAfterResume = timer.elapsed();

	EXPECT_GT(elapsedTimeAfterResume.nanoseconds(), elapsedTimeWhilePaused.nanoseconds());
	EXPECT_FALSE(timer.hasTimedOut());
}

TEST(TimerTest, HasTimedOutBecomesTrueAfterExpectedDuration)
{
	spk::Timer timer(40_ms);

	timer.start();

	spk::TimeUtils::sleepFor(spk::Duration(70_ms));

	EXPECT_TRUE(timer.hasTimedOut());
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	const spk::Duration elapsedTime = timer.elapsed();
	expectDurationAtLeast(elapsedTime, 30'000'000LL);
}

TEST(TimerTest, StateReportsTimedOutAfterExpectedDurationEvenWithoutExplicitHasTimedOutCall)
{
	spk::Timer timer(40_ms);

	timer.start();

	spk::TimeUtils::sleepFor(spk::Duration(70_ms));

	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);
}

TEST(TimerTest, TimedOutStateFreezesElapsedTime)
{
	spk::Timer timer(40_ms);

	timer.start();

	spk::TimeUtils::sleepFor(spk::Duration(70_ms));

	const spk::Duration elapsedTimeAtTimeout = timer.elapsed();
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	spk::TimeUtils::sleepFor(spk::Duration(30_ms));

	const spk::Duration elapsedTimeAfterWait = timer.elapsed();

	EXPECT_EQ(elapsedTimeAfterWait.nanoseconds(), elapsedTimeAtTimeout.nanoseconds());
}

TEST(TimerTest, StartAfterTimeoutRestartsTimerFromZero)
{
	spk::Timer timer(40_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(70_ms));

	EXPECT_TRUE(timer.hasTimedOut());
	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	timer.start();

	EXPECT_EQ(timer.state(), spk::Timer::State::Running);
	EXPECT_FALSE(timer.hasTimedOut());

	spk::TimeUtils::sleepFor(spk::Duration(15_ms));

	const spk::Duration elapsedTime = timer.elapsed();
	expectDurationBetween(elapsedTime, 5'000'000LL, 150'000'000LL);
}

TEST(TimerTest, PauseDoesNothingWhenIdle)
{
	spk::Timer timer(100_ms);

	timer.pause();

	EXPECT_EQ(timer.state(), spk::Timer::State::Idle);
	EXPECT_EQ(timer.elapsed().nanoseconds(), 0LL);
	EXPECT_FALSE(timer.hasTimedOut());
}

TEST(TimerTest, ResumeDoesNothingWhenIdle)
{
	spk::Timer timer(100_ms);

	timer.resume();

	EXPECT_EQ(timer.state(), spk::Timer::State::Idle);
	EXPECT_EQ(timer.elapsed().nanoseconds(), 0LL);
	EXPECT_FALSE(timer.hasTimedOut());
}

TEST(TimerTest, ResumeDoesNothingWhenAlreadyRunning)
{
	spk::Timer timer(200_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	const spk::Duration elapsedTimeBeforeResume = timer.elapsed();

	timer.resume();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	const spk::Duration elapsedTimeAfterResume = timer.elapsed();

	EXPECT_EQ(timer.state(), spk::Timer::State::Running);
	EXPECT_GT(elapsedTimeAfterResume.nanoseconds(), elapsedTimeBeforeResume.nanoseconds());
}

TEST(TimerTest, PauseDoesNothingWhenTimedOut)
{
	spk::Timer timer(30_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(60_ms));

	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	const spk::Duration elapsedTimeBeforePause = timer.elapsed();

	timer.pause();

	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);
	EXPECT_EQ(timer.elapsed().nanoseconds(), elapsedTimeBeforePause.nanoseconds());
}

TEST(TimerTest, ResumeDoesNothingWhenTimedOut)
{
	spk::Timer timer(30_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(60_ms));

	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);

	const spk::Duration elapsedTimeBeforeResume = timer.elapsed();

	timer.resume();

	EXPECT_EQ(timer.state(), spk::Timer::State::TimedOut);
	EXPECT_EQ(timer.elapsed().nanoseconds(), elapsedTimeBeforeResume.nanoseconds());
}

TEST(TimerTest, ElapsedRatioIsZeroWhenExpectedDurationIsZero)
{
	spk::Timer timer;

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	EXPECT_FLOAT_EQ(timer.elapsedRatio(), 0.0f);
}

TEST(TimerTest, ElapsedRatioGrowsWhileRunning)
{
	spk::Timer timer(200_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(40_ms));

	const float ratio = timer.elapsedRatio();

	EXPECT_GT(ratio, 0.0f);
	EXPECT_LT(ratio, 1.0f);
}

TEST(TimerTest, ElapsedRatioClampsToOneWhenTimedOut)
{
	spk::Timer timer(40_ms);

	timer.start();
	spk::TimeUtils::sleepFor(spk::Duration(70_ms));

	EXPECT_TRUE(timer.hasTimedOut());
	EXPECT_FLOAT_EQ(timer.elapsedRatio(), 1.0f);
}

TEST(TimerTest, ToStringReturnsExpectedValues)
{
	EXPECT_STREQ(spk::toString(spk::Timer::State::Idle), "Idle");
	EXPECT_STREQ(spk::toString(spk::Timer::State::Running), "Running");
	EXPECT_STREQ(spk::toString(spk::Timer::State::Paused), "Paused");
	EXPECT_STREQ(spk::toString(spk::Timer::State::TimedOut), "TimedOut");

	EXPECT_STREQ(spk::toWstring(spk::Timer::State::Idle), L"Idle");
	EXPECT_STREQ(spk::toWstring(spk::Timer::State::Running), L"Running");
	EXPECT_STREQ(spk::toWstring(spk::Timer::State::Paused), L"Paused");
	EXPECT_STREQ(spk::toWstring(spk::Timer::State::TimedOut), L"TimedOut");
}

TEST(TimerTest, StreamOperatorsRenderStateNames)
{
	std::ostringstream outputStream;
	outputStream << spk::Timer::State::TimedOut;
	EXPECT_EQ(outputStream.str(), "TimedOut");

	std::wostringstream wideOutputStream;
	wideOutputStream << spk::Timer::State::Paused;
	EXPECT_EQ(wideOutputStream.str(), L"Paused");
}