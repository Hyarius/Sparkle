#include <gtest/gtest.h>

#include <chrono>
#include <sstream>
#include <thread>

#include "spk_chronometer.hpp"

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

TEST(ChronometerTest, DefaultConstructionStartsIdleWithZeroElapsedTime)
{
	spk::Chronometer chronometer;

	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Idle);
	EXPECT_EQ(chronometer.elapsedTime().nanoseconds(), 0LL);
}

TEST(ChronometerTest, StartTransitionsToRunningAndElapsedTimeGrows)
{
	spk::Chronometer chronometer;

	chronometer.start();
	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Running);

	spk::TimeUtils::sleepFor(spk::Duration(30_ms));

	const spk::Duration elapsedTime = chronometer.elapsedTime();
	expectDurationAtLeast(elapsedTime, 10'000'000LL);
}

TEST(ChronometerTest, PauseFreezesElapsedTime)
{
	spk::Chronometer chronometer;

	chronometer.start();
	spk::TimeUtils::sleepFor(spk::Duration(25_ms));
	chronometer.pause();

	const spk::Duration elapsedTimeBeforeWait = chronometer.elapsedTime();
	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Paused);

	spk::TimeUtils::sleepFor(spk::Duration(25_ms));

	const spk::Duration elapsedTimeAfterWait = chronometer.elapsedTime();

	expectDurationBetween(elapsedTimeBeforeWait, 10'000'000LL, 200'000'000LL);
	EXPECT_EQ(elapsedTimeAfterWait.nanoseconds(), elapsedTimeBeforeWait.nanoseconds());
}

TEST(ChronometerTest, ResumeContinuesAccumulatingTimeAfterPause)
{
	spk::Chronometer chronometer;

	chronometer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));
	chronometer.pause();

	const spk::Duration elapsedTimeWhilePaused = chronometer.elapsedTime();

	spk::TimeUtils::sleepFor(spk::Duration(20_ms));
	EXPECT_EQ(chronometer.elapsedTime().nanoseconds(), elapsedTimeWhilePaused.nanoseconds());

	chronometer.resume();
	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Running);

	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	const spk::Duration elapsedTimeAfterResume = chronometer.elapsedTime();

	EXPECT_GT(elapsedTimeAfterResume.nanoseconds(), elapsedTimeWhilePaused.nanoseconds());
	expectDurationAtLeast(elapsedTimeAfterResume, 20'000'000LL);
}

TEST(ChronometerTest, StopResetsStateAndElapsedTime)
{
	spk::Chronometer chronometer;

	chronometer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	chronometer.stop();

	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Idle);
	EXPECT_EQ(chronometer.elapsedTime().nanoseconds(), 0LL);
}

TEST(ChronometerTest, RestartResetsElapsedTimeAndStartsRunning)
{
	spk::Chronometer chronometer;

	chronometer.start();
	spk::TimeUtils::sleepFor(spk::Duration(25_ms));

	const spk::Duration elapsedTimeBeforeRestart = chronometer.elapsedTime();
	expectDurationAtLeast(elapsedTimeBeforeRestart, 10'000'000LL);

	chronometer.restart();
	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Running);

	spk::TimeUtils::sleepFor(spk::Duration(15_ms));

	const spk::Duration elapsedTimeAfterRestart = chronometer.elapsedTime();

	expectDurationBetween(elapsedTimeAfterRestart, 5'000'000LL, 150'000'000LL);
	EXPECT_LT(elapsedTimeAfterRestart.nanoseconds(), elapsedTimeBeforeRestart.nanoseconds());
}

TEST(ChronometerTest, InvalidTransitionsDoNothing)
{
	spk::Chronometer chronometer;

	chronometer.pause();
	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Idle);
	EXPECT_EQ(chronometer.elapsedTime().nanoseconds(), 0LL);

	chronometer.resume();
	EXPECT_EQ(chronometer.state(), spk::Chronometer::State::Idle);
	EXPECT_EQ(chronometer.elapsedTime().nanoseconds(), 0LL);

	chronometer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	const spk::Duration elapsedTimeBeforeSecondStart = chronometer.elapsedTime();

	chronometer.start();
	spk::TimeUtils::sleepFor(spk::Duration(20_ms));

	const spk::Duration elapsedTimeAfterSecondStart = chronometer.elapsedTime();

	EXPECT_GT(elapsedTimeAfterSecondStart.nanoseconds(), elapsedTimeBeforeSecondStart.nanoseconds());
}

TEST(ChronometerTest, ElapsedTimeIsMonotonicWhileRunning)
{
	spk::Chronometer chronometer;

	chronometer.start();

	spk::TimeUtils::sleepFor(spk::Duration(10_ms));
	const spk::Duration elapsedTimeA = chronometer.elapsedTime();

	spk::TimeUtils::sleepFor(spk::Duration(10_ms));
	const spk::Duration elapsedTimeB = chronometer.elapsedTime();

	spk::TimeUtils::sleepFor(spk::Duration(10_ms));
	const spk::Duration elapsedTimeC = chronometer.elapsedTime();

	EXPECT_LE(elapsedTimeA.nanoseconds(), elapsedTimeB.nanoseconds());
	EXPECT_LE(elapsedTimeB.nanoseconds(), elapsedTimeC.nanoseconds());
}

TEST(ChronometerTest, ToStringReturnsExpectedValues)
{
	EXPECT_STREQ(spk::toString(spk::Chronometer::State::Idle), "Idle");
	EXPECT_STREQ(spk::toString(spk::Chronometer::State::Running), "Running");
	EXPECT_STREQ(spk::toString(spk::Chronometer::State::Paused), "Paused");

	EXPECT_STREQ(spk::toWstring(spk::Chronometer::State::Idle), L"Idle");
	EXPECT_STREQ(spk::toWstring(spk::Chronometer::State::Running), L"Running");
	EXPECT_STREQ(spk::toWstring(spk::Chronometer::State::Paused), L"Paused");
}

TEST(ChronometerTest, StreamOperatorsRenderStateNames)
{
	std::ostringstream outputStream;
	outputStream << spk::Chronometer::State::Running;
	EXPECT_EQ(outputStream.str(), "Running");

	std::wostringstream wideOutputStream;
	wideOutputStream << spk::Chronometer::State::Paused;
	EXPECT_EQ(wideOutputStream.str(), L"Paused");
}