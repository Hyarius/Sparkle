#include <gtest/gtest.h>

#include <sstream>
#include <type_traits>

#include "math/spk_vector4.hpp"

TEST(Vector4ConstructionTest, ConstructorsAndConstantsMatchVector2Style)
{
	const spk::Vector4Int zero;
	const spk::Vector4Int scalar(5);
	const spk::Vector4Int values(1, 2, 3, 4);
	const spk::Vector4Int fromVector3(spk::Vector3Int(7, 8, 9), 10);
	const spk::Vector4Int fromVector2(spk::Vector2Int(7, 8), 9, 10);
	const spk::Vector4Int converted(spk::Vector4(1.5f, 2.5f, 3.5f, 4.5f));

	EXPECT_TRUE(zero.isZero());
	EXPECT_EQ(scalar, spk::Vector4Int(5, 5, 5, 5));
	EXPECT_EQ(values, spk::Vector4Int(1, 2, 3, 4));
	EXPECT_EQ(fromVector3, spk::Vector4Int(7, 8, 9, 10));
	EXPECT_EQ(fromVector2, spk::Vector4Int(7, 8, 9, 10));
	EXPECT_EQ(converted, spk::Vector4Int(1, 2, 3, 4));
	EXPECT_EQ(spk::Vector4Int::Zero, spk::Vector4Int(0, 0, 0, 0));
	EXPECT_EQ(spk::Vector4Int::One, spk::Vector4Int(1, 1, 1, 1));
}

TEST(Vector4StringTest, FormatsAsExpected)
{
	const spk::Vector4Int value(1, -2, 3, -4);
	std::ostringstream outputStream;
	outputStream << value;

	EXPECT_EQ(value.toString(), "(1, -2, 3, -4)");
	EXPECT_EQ(outputStream.str(), "(1, -2, 3, -4)");
}

TEST(Vector4ArithmeticTest, SupportsVectorScalarAndCompoundOperations)
{
	spk::Vector4Int value(8, 12, 16, 20);

	EXPECT_EQ(value + spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(9, 14, 19, 24));
	EXPECT_EQ(value - spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(7, 10, 13, 16));
	EXPECT_EQ(value * spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(8, 24, 48, 80));
	EXPECT_EQ(value / spk::Vector4Int(2, 3, 4, 5), spk::Vector4Int(4, 4, 4, 4));
	EXPECT_EQ(value + 2, spk::Vector4Int(10, 14, 18, 22));
	EXPECT_EQ(2 + value, spk::Vector4Int(10, 14, 18, 22));
	EXPECT_EQ(20 - spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(19, 18, 17, 16));
	EXPECT_EQ(2 * spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(2, 4, 6, 8));
	EXPECT_EQ(24 / spk::Vector4Int(2, 3, 4, 6), spk::Vector4Int(12, 8, 6, 4));

	value += spk::Vector4Int(1, 1, 1, 1);
	value -= 1;
	value *= 2;
	value /= 2;
	EXPECT_EQ(value, spk::Vector4Int(8, 12, 16, 20));
}

TEST(Vector4ArithmeticTest, DivisionRejectsZero)
{
	EXPECT_THROW(static_cast<void>(spk::Vector4Int(1, 2, 3, 4) / spk::Vector4Int(1, 0, 1, 1)), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(spk::Vector4Int(1, 2, 3, 4) / 0), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(20 / spk::Vector4Int(1, 0, 1, 1)), std::invalid_argument);
}

TEST(Vector4ArithmeticTest, SupportsFloatOperationsUsedBySIMDPath)
{
	spk::Vector4 value(8.0f, 12.0f, 16.0f, 20.0f);

	EXPECT_EQ(value + spk::Vector4(1.0f, 2.0f, 3.0f, 4.0f), spk::Vector4(9.0f, 14.0f, 19.0f, 24.0f));
	EXPECT_EQ(value - spk::Vector4(1.0f), spk::Vector4(7.0f, 11.0f, 15.0f, 19.0f));
	EXPECT_EQ(value * 0.5f, spk::Vector4(4.0f, 6.0f, 8.0f, 10.0f));
	EXPECT_EQ(value / spk::Vector4(2.0f, 3.0f, 4.0f, 5.0f), spk::Vector4(4.0f));
	EXPECT_FLOAT_EQ(value.dot(spk::Vector4(1.0f, 2.0f, 3.0f, 4.0f)), 160.0f);
	EXPECT_FLOAT_EQ(value.squaredLength(), 864.0f);

	value += 2.0f;
	value *= spk::Vector4(2.0f);
	EXPECT_EQ(value, spk::Vector4(20.0f, 28.0f, 36.0f, 44.0f));
}

TEST(Vector4GeometryTest, SupportsGeometryHelpers)
{
	const spk::Vector4Int value(1, 2, 4, 8);

	EXPECT_EQ(-value, spk::Vector4Int(-1, -2, -4, -8));
	EXPECT_FLOAT_EQ(value.squaredLength(), 85.0f);
	EXPECT_FLOAT_EQ(value.squaredDistance(spk::Vector4Int(2, 4, 8, 16)), 85.0f);
	EXPECT_FLOAT_EQ(value.dot(spk::Vector4Int(1, 2, 3, 4)), 49.0f);
	EXPECT_EQ(value.xy(), spk::Vector2Int(1, 2));
	EXPECT_EQ(value.xyz(), spk::Vector3Int(1, 2, 4));
	EXPECT_NEAR(value.normalized().length(), 1.0f, 0.000001f);
	EXPECT_THROW(static_cast<void>(spk::Vector4Int::Zero.normalized()), std::runtime_error);
}

TEST(Vector4UtilityTest, SupportsLerpMinMaxClampBetweenAndConversion)
{
	EXPECT_EQ(spk::Vector4::lerp(spk::Vector4(0.0f, 0.0f, 0.0f, 0.0f), spk::Vector4(10.0f, 20.0f, 30.0f, 40.0f), 0.5f), spk::Vector4(5.0f, 10.0f, 15.0f, 20.0f));
	EXPECT_EQ(spk::Vector4Int::min(spk::Vector4Int(5, 2, 9, 8), spk::Vector4Int(3, 4, 1, 10)), spk::Vector4Int(3, 2, 1, 8));
	EXPECT_EQ(spk::Vector4Int::max({spk::Vector4Int(5, 2, 9, 8), spk::Vector4Int(3, 4, 1, 10)}), spk::Vector4Int(5, 4, 9, 10));
	EXPECT_THROW(static_cast<void>(spk::Vector4Int::max({})), std::invalid_argument);
	EXPECT_EQ(spk::Vector4Int::clamp(spk::Vector4Int(5, -2, 3, 4), spk::Vector4Int(0, 0, 0, 0), spk::Vector4Int(2, 2, 2, 2)), spk::Vector4Int(2, 0, 2, 2));
	EXPECT_TRUE(spk::Vector4Int::isBetween(spk::Vector4Int(1, 2, 3, 4), spk::Vector4Int(0, 0, 0, 0), spk::Vector4Int(4, 4, 4, 4)));

	const spk::IVector4<float> converted = static_cast<spk::IVector4<float>>(spk::Vector4Int(1, 2, 3, 4));
	EXPECT_FLOAT_EQ(converted.w, 4.0f);
	static_assert(std::is_same_v<spk::Vector4, spk::IVector4<float>>);
}
