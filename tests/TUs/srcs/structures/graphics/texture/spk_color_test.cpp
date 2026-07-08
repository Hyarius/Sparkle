#include <gtest/gtest.h>

#include <sstream>

#include "structures/graphics/geometry/spk_color.hpp"

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

TEST(ColorTest, HexConstructorParsesRgb)
{
	spk::Color color("#5FB0B7");
	EXPECT_FLOAT_EQ(color.r, static_cast<float>(0x5F) / 255.0f);
	EXPECT_FLOAT_EQ(color.g, static_cast<float>(0xB0) / 255.0f);
	EXPECT_FLOAT_EQ(color.b, static_cast<float>(0xB7) / 255.0f);
	EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST(ColorTest, HexConstructorParsesRgba)
{
	spk::Color color("#5fb0b780");
	EXPECT_FLOAT_EQ(color.r, static_cast<float>(0x5F) / 255.0f);
	EXPECT_FLOAT_EQ(color.g, static_cast<float>(0xB0) / 255.0f);
	EXPECT_FLOAT_EQ(color.b, static_cast<float>(0xB7) / 255.0f);
	EXPECT_FLOAT_EQ(color.a, static_cast<float>(0x80) / 255.0f);
}

TEST(ColorTest, HexConstructorParsesBounds)
{
	spk::Color black("#000000");
	EXPECT_FLOAT_EQ(black.r, 0.0f);
	EXPECT_FLOAT_EQ(black.a, 1.0f);
	spk::Color white("#FFFFFFFF");
	EXPECT_FLOAT_EQ(white.r, 1.0f);
	EXPECT_FLOAT_EQ(white.g, 1.0f);
	EXPECT_FLOAT_EQ(white.b, 1.0f);
	EXPECT_FLOAT_EQ(white.a, 1.0f);
}

TEST(ColorTest, HexConstructorRejectsMalformedCodes)
{
	EXPECT_THROW(spk::Color("5FB0B7"), std::invalid_argument);
	EXPECT_THROW(spk::Color("#5FB0"), std::invalid_argument);
	EXPECT_THROW(spk::Color("#5FB0B7F"), std::invalid_argument);
	EXPECT_THROW(spk::Color("#GGB0B7"), std::invalid_argument);
	EXPECT_THROW(spk::Color(""), std::invalid_argument);
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
