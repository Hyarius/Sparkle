#include <gtest/gtest.h>

#include <sstream>

#include "type/spk_vertical_alignment.hpp"

TEST(VerticalAlignmentTest, ToStringTop)
{
	EXPECT_EQ(spk::toString(spk::VerticalAlignment::Top), "Top");
}

TEST(VerticalAlignmentTest, ToStringCentered)
{
	EXPECT_EQ(spk::toString(spk::VerticalAlignment::Centered), "Centered");
}

TEST(VerticalAlignmentTest, ToStringDown)
{
	EXPECT_EQ(spk::toString(spk::VerticalAlignment::Down), "Down");
}

TEST(VerticalAlignmentTest, ToStringUnknownFallback)
{
	EXPECT_EQ(spk::toString(static_cast<spk::VerticalAlignment>(999)), "Unknown VerticalAlignment");
}

TEST(VerticalAlignmentTest, ToWstringTop)
{
	EXPECT_EQ(spk::toWstring(spk::VerticalAlignment::Top), L"Top");
}

TEST(VerticalAlignmentTest, ToWstringCentered)
{
	EXPECT_EQ(spk::toWstring(spk::VerticalAlignment::Centered), L"Centered");
}

TEST(VerticalAlignmentTest, ToWstringDown)
{
	EXPECT_EQ(spk::toWstring(spk::VerticalAlignment::Down), L"Down");
}

TEST(VerticalAlignmentTest, ToWstringUnknownFallback)
{
	EXPECT_EQ(spk::toWstring(static_cast<spk::VerticalAlignment>(999)), L"Unknown VerticalAlignment");
}

TEST(VerticalAlignmentTest, OstreamOperatorTop)
{
	std::ostringstream stream;
	stream << spk::VerticalAlignment::Top;
	EXPECT_EQ(stream.str(), "Top");
}

TEST(VerticalAlignmentTest, OstreamOperatorCentered)
{
	std::ostringstream stream;
	stream << spk::VerticalAlignment::Centered;
	EXPECT_EQ(stream.str(), "Centered");
}

TEST(VerticalAlignmentTest, OstreamOperatorDown)
{
	std::ostringstream stream;
	stream << spk::VerticalAlignment::Down;
	EXPECT_EQ(stream.str(), "Down");
}

TEST(VerticalAlignmentTest, WostreamOperatorTop)
{
	std::wostringstream stream;
	stream << spk::VerticalAlignment::Top;
	EXPECT_EQ(stream.str(), L"Top");
}

TEST(VerticalAlignmentTest, WostreamOperatorDown)
{
	std::wostringstream stream;
	stream << spk::VerticalAlignment::Down;
	EXPECT_EQ(stream.str(), L"Down");
}
