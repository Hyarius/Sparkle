#include <gtest/gtest.h>

#include <sstream>

#include "type/spk_horizontal_alignment.hpp"

TEST(HorizontalAlignmentTest, ToStringLeft)
{
	EXPECT_EQ(spk::toString(spk::HorizontalAlignment::Left), "Left");
}

TEST(HorizontalAlignmentTest, ToStringCentered)
{
	EXPECT_EQ(spk::toString(spk::HorizontalAlignment::Centered), "Centered");
}

TEST(HorizontalAlignmentTest, ToStringRight)
{
	EXPECT_EQ(spk::toString(spk::HorizontalAlignment::Right), "Right");
}

TEST(HorizontalAlignmentTest, ToStringUnknownFallback)
{
	EXPECT_EQ(spk::toString(static_cast<spk::HorizontalAlignment>(999)), "Unknown HorizontalAlignment");
}

TEST(HorizontalAlignmentTest, ToWstringLeft)
{
	EXPECT_EQ(spk::toWstring(spk::HorizontalAlignment::Left), L"Left");
}

TEST(HorizontalAlignmentTest, ToWstringCentered)
{
	EXPECT_EQ(spk::toWstring(spk::HorizontalAlignment::Centered), L"Centered");
}

TEST(HorizontalAlignmentTest, ToWstringRight)
{
	EXPECT_EQ(spk::toWstring(spk::HorizontalAlignment::Right), L"Right");
}

TEST(HorizontalAlignmentTest, ToWstringUnknownFallback)
{
	EXPECT_EQ(spk::toWstring(static_cast<spk::HorizontalAlignment>(999)), L"Unknown HorizontalAlignment");
}

TEST(HorizontalAlignmentTest, OstreamOperatorLeft)
{
	std::ostringstream stream;
	stream << spk::HorizontalAlignment::Left;
	EXPECT_EQ(stream.str(), "Left");
}

TEST(HorizontalAlignmentTest, OstreamOperatorCentered)
{
	std::ostringstream stream;
	stream << spk::HorizontalAlignment::Centered;
	EXPECT_EQ(stream.str(), "Centered");
}

TEST(HorizontalAlignmentTest, OstreamOperatorRight)
{
	std::ostringstream stream;
	stream << spk::HorizontalAlignment::Right;
	EXPECT_EQ(stream.str(), "Right");
}

TEST(HorizontalAlignmentTest, WostreamOperatorLeft)
{
	std::wostringstream stream;
	stream << spk::HorizontalAlignment::Left;
	EXPECT_EQ(stream.str(), L"Left");
}

TEST(HorizontalAlignmentTest, WostreamOperatorRight)
{
	std::wostringstream stream;
	stream << spk::HorizontalAlignment::Right;
	EXPECT_EQ(stream.str(), L"Right");
}
