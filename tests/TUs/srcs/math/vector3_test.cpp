#include <gtest/gtest.h>

#include <sstream>
#include <type_traits>

#include "math/spk_vector3.hpp"

TEST(Vector3ConstructionTest, ConstructorsAndConstantsMatchVector2Style)
{
	const spk::Vector3Int zero;
	const spk::Vector3Int scalar(5);
	const spk::Vector3Int values(1, 2, 3);
	const spk::Vector3Int fromVector2(spk::Vector2Int(7, 8), 9);
	const spk::Vector3Int converted(spk::Vector3(1.5f, 2.5f, 3.5f));

	EXPECT_TRUE(zero.isZero());
	EXPECT_EQ(scalar, spk::Vector3Int(5, 5, 5));
	EXPECT_EQ(values, spk::Vector3Int(1, 2, 3));
	EXPECT_EQ(fromVector2, spk::Vector3Int(7, 8, 9));
	EXPECT_EQ(converted, spk::Vector3Int(1, 2, 3));
	EXPECT_EQ(spk::Vector3Int::Zero, spk::Vector3Int(0, 0, 0));
	EXPECT_EQ(spk::Vector3Int::One, spk::Vector3Int(1, 1, 1));
}

TEST(Vector3StringTest, FormatsAsExpected)
{
	const spk::Vector3Int value(1, -2, 3);
	std::ostringstream outputStream;
	outputStream << value;

	EXPECT_EQ(value.toString(), "(1, -2, 3)");
	EXPECT_EQ(outputStream.str(), "(1, -2, 3)");
}

TEST(Vector3ArithmeticTest, SupportsVectorScalarAndCompoundOperations)
{
	spk::Vector3Int value(8, 12, 16);

	EXPECT_EQ(value + spk::Vector3Int(1, 2, 3), spk::Vector3Int(9, 14, 19));
	EXPECT_EQ(value - spk::Vector3Int(1, 2, 3), spk::Vector3Int(7, 10, 13));
	EXPECT_EQ(value * spk::Vector3Int(1, 2, 3), spk::Vector3Int(8, 24, 48));
	EXPECT_EQ(value / spk::Vector3Int(2, 3, 4), spk::Vector3Int(4, 4, 4));
	EXPECT_EQ(value + 2, spk::Vector3Int(10, 14, 18));
	EXPECT_EQ(2 + value, spk::Vector3Int(10, 14, 18));
	EXPECT_EQ(20 - spk::Vector3Int(1, 2, 3), spk::Vector3Int(19, 18, 17));
	EXPECT_EQ(2 * spk::Vector3Int(1, 2, 3), spk::Vector3Int(2, 4, 6));
	EXPECT_EQ(24 / spk::Vector3Int(2, 3, 4), spk::Vector3Int(12, 8, 6));

	value += spk::Vector3Int(1, 1, 1);
	value -= 1;
	value *= 2;
	value /= 2;
	EXPECT_EQ(value, spk::Vector3Int(8, 12, 16));
}

TEST(Vector3ArithmeticTest, DivisionRejectsZero)
{
	EXPECT_THROW(static_cast<void>(spk::Vector3Int(1, 2, 3) / spk::Vector3Int(1, 0, 1)), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(spk::Vector3Int(1, 2, 3) / 0), std::invalid_argument);
	EXPECT_THROW(static_cast<void>(20 / spk::Vector3Int(1, 0, 1)), std::invalid_argument);
}

TEST(Vector3GeometryTest, SupportsGeometryHelpers)
{
	const spk::Vector3Int value(2, 3, 6);

	EXPECT_EQ(-value, spk::Vector3Int(-2, -3, -6));
	EXPECT_FLOAT_EQ(value.squaredLength(), 49.0f);
	EXPECT_FLOAT_EQ(value.length(), 7.0f);
	EXPECT_FLOAT_EQ(value.squaredDistance(spk::Vector3Int(4, 6, 10)), 29.0f);
	EXPECT_FLOAT_EQ(value.dot(spk::Vector3Int(1, 2, 3)), 26.0f);
	EXPECT_EQ(spk::Vector3Int(1, 0, 0).cross(spk::Vector3Int(0, 1, 0)), spk::Vector3Int(0, 0, 1));
	EXPECT_EQ(value.xy(), spk::Vector2Int(2, 3));
	EXPECT_EQ(value.xz(), spk::Vector2Int(2, 6));
	EXPECT_EQ(value.yz(), spk::Vector2Int(3, 6));
	EXPECT_NEAR(value.normalized().length(), 1.0f, 0.000001f);
	EXPECT_THROW(static_cast<void>(spk::Vector3Int::Zero.normalized()), std::runtime_error);
}

TEST(Vector3UtilityTest, SupportsLerpMinMaxClampBetweenAndConversion)
{
	EXPECT_EQ(spk::Vector3::lerp(spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(10.0f, 20.0f, 30.0f), 0.5f), spk::Vector3(5.0f, 10.0f, 15.0f));
	EXPECT_EQ(spk::Vector3Int::min(spk::Vector3Int(5, 2, 9), spk::Vector3Int(3, 4, 1)), spk::Vector3Int(3, 2, 1));
	EXPECT_EQ(spk::Vector3Int::max({spk::Vector3Int(5, 2, 9), spk::Vector3Int(3, 4, 1)}), spk::Vector3Int(5, 4, 9));
	EXPECT_THROW(static_cast<void>(spk::Vector3Int::min({})), std::invalid_argument);
	EXPECT_EQ(spk::Vector3Int::clamp(spk::Vector3Int(5, -2, 3), spk::Vector3Int(0, 0, 0), spk::Vector3Int(2, 2, 2)), spk::Vector3Int(2, 0, 2));
	EXPECT_TRUE(spk::Vector3Int::isBetween(spk::Vector3Int(1, 2, 3), spk::Vector3Int(0, 0, 0), spk::Vector3Int(3, 3, 3)));

	const spk::IVector3<float> converted = static_cast<spk::IVector3<float>>(spk::Vector3Int(1, 2, 3));
	EXPECT_FLOAT_EQ(converted.z, 3.0f);
	static_assert(std::is_same_v<spk::Vector3, spk::IVector3<float>>);
}
