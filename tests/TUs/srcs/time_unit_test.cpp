#include <gtest/gtest.h>

#include <sstream>

#include "time/spk_time_unit.hpp"

TEST(TimeUnitTest, ToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::TimeUnit::Second), "Second");
	EXPECT_EQ(spk::toString(spk::TimeUnit::Millisecond), "Millisecond");
	EXPECT_EQ(spk::toString(spk::TimeUnit::Nanosecond), "Nanosecond");
}

TEST(TimeUnitTest, ToStringReturnsFallbackForInvalidValue)
{
	const auto invalidTimeUnit = static_cast<spk::TimeUnit>(999);

	EXPECT_EQ(spk::toString(invalidTimeUnit), "Unknown TimeUnit");
}

TEST(TimeUnitTest, ToWstringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWstring(spk::TimeUnit::Second), L"Second");
	EXPECT_EQ(spk::toWstring(spk::TimeUnit::Millisecond), L"Millisecond");
	EXPECT_EQ(spk::toWstring(spk::TimeUnit::Nanosecond), L"Nanosecond");
}

TEST(TimeUnitTest, ToWstringReturnsFallbackForInvalidValue)
{
	const auto invalidTimeUnit = static_cast<spk::TimeUnit>(999);

	EXPECT_EQ(spk::toWstring(invalidTimeUnit), L"Unknown TimeUnit");
}

TEST(TimeUnitTest, StreamOperatorsPrintExpectedText)
{
	std::ostringstream outputStream;
	std::wostringstream wideOutputStream;

	outputStream << spk::TimeUnit::Millisecond;
	wideOutputStream << spk::TimeUnit::Nanosecond;

	EXPECT_EQ(outputStream.str(), "Millisecond");
	EXPECT_EQ(wideOutputStream.str(), L"Nanosecond");
}
