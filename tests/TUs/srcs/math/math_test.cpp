#include <gtest/gtest.h>

#include "math/spk_math.hpp"

TEST(MathTest, DegreeToRadianConvertsKnownAngles)
{
	EXPECT_NEAR(spk::degreeToRadian(0.0f), 0.0f, 0.000001f);
	EXPECT_NEAR(spk::degreeToRadian(180.0f), 3.14159265f, 0.000001f);
	EXPECT_NEAR(spk::degreeToRadian(90.0f), 1.57079632f, 0.000001f);
}

TEST(MathTest, RadianToDegreeConvertsKnownAngles)
{
	EXPECT_NEAR(spk::radianToDegree(0.0f), 0.0f, 0.000001f);
	EXPECT_NEAR(spk::radianToDegree(3.14159265f), 180.0f, 0.000001f);
	EXPECT_NEAR(spk::radianToDegree(1.57079632f), 90.0f, 0.000001f);
}
