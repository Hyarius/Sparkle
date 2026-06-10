#include <gtest/gtest.h>

#include <sstream>

#include "type/spk_orientation.hpp"

TEST(OrientationTest, ToStringHorizontal)
{
	EXPECT_EQ(spk::toString(spk::Orientation::Horizontal), "Horizontal");
}

TEST(OrientationTest, ToStringVertical)
{
	EXPECT_EQ(spk::toString(spk::Orientation::Vertical), "Vertical");
}

TEST(OrientationTest, ToStringUnknownFallback)
{
	EXPECT_EQ(spk::toString(static_cast<spk::Orientation>(999)), "Unknown Orientation");
}

TEST(OrientationTest, ToWstringHorizontal)
{
	EXPECT_EQ(spk::toWstring(spk::Orientation::Horizontal), L"Horizontal");
}

TEST(OrientationTest, ToWstringVertical)
{
	EXPECT_EQ(spk::toWstring(spk::Orientation::Vertical), L"Vertical");
}

TEST(OrientationTest, ToWstringUnknownFallback)
{
	EXPECT_EQ(spk::toWstring(static_cast<spk::Orientation>(999)), L"Unknown Orientation");
}

TEST(OrientationTest, OstreamOperatorHorizontal)
{
	std::ostringstream stream;
	stream << spk::Orientation::Horizontal;
	EXPECT_EQ(stream.str(), "Horizontal");
}

TEST(OrientationTest, OstreamOperatorVertical)
{
	std::ostringstream stream;
	stream << spk::Orientation::Vertical;
	EXPECT_EQ(stream.str(), "Vertical");
}

TEST(OrientationTest, WostreamOperatorHorizontal)
{
	std::wostringstream stream;
	stream << spk::Orientation::Horizontal;
	EXPECT_EQ(stream.str(), L"Horizontal");
}

TEST(OrientationTest, WostreamOperatorVertical)
{
	std::wostringstream stream;
	stream << spk::Orientation::Vertical;
	EXPECT_EQ(stream.str(), L"Vertical");
}
