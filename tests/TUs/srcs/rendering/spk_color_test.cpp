#include <gtest/gtest.h>

#include <sstream>

#include "rendering/spk_color.hpp"

TEST(ColorTest, DefaultConstructorSetsWhite)
{
	spk::Color color;
	EXPECT_FLOAT_EQ(color.r, 1.0f);
	EXPECT_FLOAT_EQ(color.g, 1.0f);
	EXPECT_FLOAT_EQ(color.b, 1.0f);
	EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST(ColorTest, ParameterizedConstructorSetsComponents)
{
	spk::Color color(0.1f, 0.2f, 0.3f, 0.4f);
	EXPECT_FLOAT_EQ(color.r, 0.1f);
	EXPECT_FLOAT_EQ(color.g, 0.2f);
	EXPECT_FLOAT_EQ(color.b, 0.3f);
	EXPECT_FLOAT_EQ(color.a, 0.4f);
}

TEST(ColorTest, ParameterizedConstructorDefaultAlphaIsOne)
{
	spk::Color color(0.5f, 0.6f, 0.7f);
	EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST(ColorTest, ValuesReturnsCorrectArray)
{
	spk::Color color(0.1f, 0.2f, 0.3f, 0.4f);
	auto vals = color.values();
	EXPECT_FLOAT_EQ(vals[0], 0.1f);
	EXPECT_FLOAT_EQ(vals[1], 0.2f);
	EXPECT_FLOAT_EQ(vals[2], 0.3f);
	EXPECT_FLOAT_EQ(vals[3], 0.4f);
}

TEST(ColorTest, StreamOperatorFormatsCorrectly)
{
	spk::Color color(0.1f, 0.2f, 0.3f, 0.4f);
	std::ostringstream oss;
	oss << color;
	std::string result = oss.str();
	EXPECT_FALSE(result.empty());
	EXPECT_NE(result.find("0.1"), std::string::npos);
	EXPECT_NE(result.find("0.2"), std::string::npos);
	EXPECT_NE(result.find("0.3"), std::string::npos);
	EXPECT_NE(result.find("0.4"), std::string::npos);
}

TEST(ColorTest, ToStringMatchesStreamOperator)
{
	spk::Color color(0.5f, 0.6f, 0.7f, 0.8f);
	std::ostringstream oss;
	oss << color;
	EXPECT_EQ(color.toString(), oss.str());
}
