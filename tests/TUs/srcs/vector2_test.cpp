#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <type_traits>

#include "spk_vector2.hpp"

TEST(Vector2ConstructionTest, DefaultConstructorBuildsZeroVectorInt)
{
	const spk::Vector2Int value;

	EXPECT_EQ(value.x, 0);
	EXPECT_EQ(value.y, 0);
	EXPECT_TRUE(value.isZero());
}

TEST(Vector2ConstructionTest, DefaultConstructorBuildsZeroVectorFloat)
{
	const spk::Vector2 value;

	EXPECT_FLOAT_EQ(value.x, 0.0f);
	EXPECT_FLOAT_EQ(value.y, 0.0f);
	EXPECT_TRUE(value.isZero());
}

TEST(Vector2ConstructionTest, TwoValueConstructorStoresComponents)
{
	const spk::Vector2Int value(4, -7);

	EXPECT_EQ(value.x, 4);
	EXPECT_EQ(value.y, -7);
}

TEST(Vector2ConstructionTest, ScalarConstructorDuplicatesValue)
{
	const spk::Vector2Int value(5);

	EXPECT_EQ(value.x, 5);
	EXPECT_EQ(value.y, 5);
}

TEST(Vector2ConstructionTest, ExplicitConvertingConstructorCopiesAndCasts)
{
	const spk::Vector2 source(3.5f, -7.25f);
	const spk::Vector2Int converted(source);

	EXPECT_EQ(converted.x, 3);
	EXPECT_EQ(converted.y, -7);
}

TEST(Vector2ConstructionTest, ZeroConstantIsZero)
{
	EXPECT_EQ(spk::Vector2Int::Zero.x, 0);
	EXPECT_EQ(spk::Vector2Int::Zero.y, 0);

	EXPECT_FLOAT_EQ(spk::Vector2::Zero.x, 0.0f);
	EXPECT_FLOAT_EQ(spk::Vector2::Zero.y, 0.0f);
}

TEST(Vector2ConstructionTest, OneConstantIsOne)
{
	EXPECT_EQ(spk::Vector2Int::One.x, 1);
	EXPECT_EQ(spk::Vector2Int::One.y, 1);

	EXPECT_FLOAT_EQ(spk::Vector2::One.x, 1.0f);
	EXPECT_FLOAT_EQ(spk::Vector2::One.y, 1.0f);
}

TEST(Vector2StringTest, ToStringFormatsAsExpected)
{
	const spk::Vector2Int value(4, -7);

	EXPECT_EQ(value.toString(), "(4, -7)");
}

TEST(Vector2StringTest, StreamOutputFormatsAsExpected)
{
	const spk::Vector2Int value(12, 34);

	std::ostringstream outputStream;
	outputStream << value;

	EXPECT_EQ(outputStream.str(), "(12, 34)");
}

TEST(Vector2EqualityTest, IntegerEqualityWorksExactly)
{
	const spk::Vector2Int left(1, 2);
	const spk::Vector2Int right(1, 2);
	const spk::Vector2Int different(1, 3);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);

	EXPECT_FALSE(left == different);
	EXPECT_TRUE(left != different);
}

TEST(Vector2EqualityTest, FloatEqualityUsesApproximateComparison)
{
	const spk::Vector2 left(1.0f, 2.0f);
	const spk::Vector2 right(1.0f, 2.0f);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);
}

TEST(Vector2ArithmeticTest, VectorAdditionWorks)
{
	const spk::Vector2Int left(1, 2);
	const spk::Vector2Int right(3, 4);

	const spk::Vector2Int result = left + right;

	EXPECT_EQ(result.x, 4);
	EXPECT_EQ(result.y, 6);
}

TEST(Vector2ArithmeticTest, VectorSubtractionWorks)
{
	const spk::Vector2Int left(8, 6);
	const spk::Vector2Int right(3, 2);

	const spk::Vector2Int result = left - right;

	EXPECT_EQ(result.x, 5);
	EXPECT_EQ(result.y, 4);
}

TEST(Vector2ArithmeticTest, VectorMultiplicationWorksComponentWise)
{
	const spk::Vector2Int left(2, 3);
	const spk::Vector2Int right(4, 5);

	const spk::Vector2Int result = left * right;

	EXPECT_EQ(result.x, 8);
	EXPECT_EQ(result.y, 15);
}

TEST(Vector2ArithmeticTest, VectorDivisionWorksComponentWise)
{
	const spk::Vector2Int left(8, 15);
	const spk::Vector2Int right(2, 5);

	const spk::Vector2Int result = left / right;

	EXPECT_EQ(result.x, 4);
	EXPECT_EQ(result.y, 3);
}

TEST(Vector2ArithmeticTest, VectorDivisionThrowsOnZeroX)
{
	const spk::Vector2Int left(8, 15);
	const spk::Vector2Int right(0, 5);

	EXPECT_THROW(static_cast<void>(left / right), std::invalid_argument);
}

TEST(Vector2ArithmeticTest, VectorDivisionThrowsOnZeroY)
{
	const spk::Vector2Int left(8, 15);
	const spk::Vector2Int right(2, 0);

	EXPECT_THROW(static_cast<void>(left / right), std::invalid_argument);
}

TEST(Vector2ArithmeticTest, ScalarAdditionWorks)
{
	const spk::Vector2Int value(2, 3);
	const spk::Vector2Int result = value + 10;

	EXPECT_EQ(result.x, 12);
	EXPECT_EQ(result.y, 13);
}

TEST(Vector2ArithmeticTest, ScalarSubtractionWorks)
{
	const spk::Vector2Int value(12, 13);
	const spk::Vector2Int result = value - 10;

	EXPECT_EQ(result.x, 2);
	EXPECT_EQ(result.y, 3);
}

TEST(Vector2ArithmeticTest, ScalarMultiplicationWorks)
{
	const spk::Vector2Int value(2, 3);
	const spk::Vector2Int result = value * 4;

	EXPECT_EQ(result.x, 8);
	EXPECT_EQ(result.y, 12);
}

TEST(Vector2ArithmeticTest, ScalarDivisionWorks)
{
	const spk::Vector2Int value(8, 12);
	const spk::Vector2Int result = value / 4;

	EXPECT_EQ(result.x, 2);
	EXPECT_EQ(result.y, 3);
}

TEST(Vector2ArithmeticTest, ScalarDivisionThrowsOnZero)
{
	const spk::Vector2Int value(8, 12);

	EXPECT_THROW(static_cast<void>(value / 0), std::invalid_argument);
}

TEST(Vector2ArithmeticTest, LeftScalarAdditionWorks)
{
	const spk::Vector2Int value(2, 3);
	const spk::Vector2Int result = 10 + value;

	EXPECT_EQ(result.x, 12);
	EXPECT_EQ(result.y, 13);
}

TEST(Vector2ArithmeticTest, LeftScalarSubtractionWorks)
{
	const spk::Vector2Int value(2, 3);
	const spk::Vector2Int result = 10 - value;

	EXPECT_EQ(result.x, 8);
	EXPECT_EQ(result.y, 7);
}

TEST(Vector2ArithmeticTest, LeftScalarMultiplicationWorks)
{
	const spk::Vector2Int value(2, 3);
	const spk::Vector2Int result = 4 * value;

	EXPECT_EQ(result.x, 8);
	EXPECT_EQ(result.y, 12);
}

TEST(Vector2ArithmeticTest, LeftScalarDivisionWorks)
{
	const spk::Vector2Int value(2, 5);
	const spk::Vector2Int result = 20 / value;

	EXPECT_EQ(result.x, 10);
	EXPECT_EQ(result.y, 4);
}

TEST(Vector2ArithmeticTest, LeftScalarDivisionThrowsOnZeroComponent)
{
	const spk::Vector2Int value(2, 0);

	EXPECT_THROW(static_cast<void>(20 / value), std::invalid_argument);
}

TEST(Vector2UnaryTest, InverseNegatesComponents)
{
	const spk::Vector2Int value(2, -3);
	const spk::Vector2Int result = value.inverse();

	EXPECT_EQ(result.x, -2);
	EXPECT_EQ(result.y, 3);
}

TEST(Vector2UnaryTest, UnaryMinusNegatesComponents)
{
	const spk::Vector2Int value(2, -3);
	const spk::Vector2Int result = -value;

	EXPECT_EQ(result.x, -2);
	EXPECT_EQ(result.y, 3);
}

TEST(Vector2CompoundAssignmentTest, VectorPlusEqualsWorks)
{
	spk::Vector2Int value(1, 2);
	value += spk::Vector2Int(3, 4);

	EXPECT_EQ(value.x, 4);
	EXPECT_EQ(value.y, 6);
}

TEST(Vector2CompoundAssignmentTest, VectorMinusEqualsWorks)
{
	spk::Vector2Int value(8, 6);
	value -= spk::Vector2Int(3, 2);

	EXPECT_EQ(value.x, 5);
	EXPECT_EQ(value.y, 4);
}

TEST(Vector2CompoundAssignmentTest, VectorMultiplyEqualsWorks)
{
	spk::Vector2Int value(2, 3);
	value *= spk::Vector2Int(4, 5);

	EXPECT_EQ(value.x, 8);
	EXPECT_EQ(value.y, 15);
}

TEST(Vector2CompoundAssignmentTest, VectorDivideEqualsWorks)
{
	spk::Vector2Int value(8, 15);
	value /= spk::Vector2Int(2, 5);

	EXPECT_EQ(value.x, 4);
	EXPECT_EQ(value.y, 3);
}

TEST(Vector2CompoundAssignmentTest, VectorDivideEqualsThrowsOnZeroComponent)
{
	spk::Vector2Int value(8, 15);

	EXPECT_THROW(value /= spk::Vector2Int(0, 5), std::invalid_argument);
}

TEST(Vector2CompoundAssignmentTest, ScalarPlusEqualsWorks)
{
	spk::Vector2Int value(1, 2);
	value += 10;

	EXPECT_EQ(value.x, 11);
	EXPECT_EQ(value.y, 12);
}

TEST(Vector2CompoundAssignmentTest, ScalarMinusEqualsWorks)
{
	spk::Vector2Int value(11, 12);
	value -= 10;

	EXPECT_EQ(value.x, 1);
	EXPECT_EQ(value.y, 2);
}

TEST(Vector2CompoundAssignmentTest, ScalarMultiplyEqualsWorks)
{
	spk::Vector2Int value(2, 3);
	value *= 4;

	EXPECT_EQ(value.x, 8);
	EXPECT_EQ(value.y, 12);
}

TEST(Vector2CompoundAssignmentTest, ScalarDivideEqualsWorks)
{
	spk::Vector2Int value(8, 12);
	value /= 4;

	EXPECT_EQ(value.x, 2);
	EXPECT_EQ(value.y, 3);
}

TEST(Vector2CompoundAssignmentTest, ScalarDivideEqualsThrowsOnZero)
{
	spk::Vector2Int value(8, 12);

	EXPECT_THROW(value /= 0, std::invalid_argument);
}

TEST(Vector2ZeroTest, IsZeroReturnsTrueForZeroVector)
{
	const spk::Vector2Int value(0, 0);

	EXPECT_TRUE(value.isZero());
}

TEST(Vector2ZeroTest, IsZeroReturnsFalseForNonZeroVector)
{
	const spk::Vector2Int value(0, 1);

	EXPECT_FALSE(value.isZero());
}

TEST(Vector2GeometryTest, SquaredLengthWorksForIntVector)
{
	const spk::Vector2Int value(3, 4);

	EXPECT_FLOAT_EQ(value.squaredLength(), 25.0f);
}

TEST(Vector2GeometryTest, LengthWorksForIntVector)
{
	const spk::Vector2Int value(3, 4);

	EXPECT_FLOAT_EQ(value.length(), 5.0f);
}

TEST(Vector2GeometryTest, NormalizedReturnsUnitVector)
{
	const spk::Vector2Int value(3, 4);
	const spk::IVector2<float> result = value.normalized();

	EXPECT_NEAR(result.x, 0.6f, 1e-6f);
	EXPECT_NEAR(result.y, 0.8f, 1e-6f);
	EXPECT_NEAR(result.length(), 1.0f, 1e-6f);
}

TEST(Vector2GeometryTest, NormalizedThrowsForZeroVector)
{
	const spk::Vector2Int value(0, 0);

	EXPECT_THROW(static_cast<void>(value.normalized()), std::runtime_error);
}

TEST(Vector2GeometryTest, SquaredDistanceWorks)
{
	const spk::Vector2Int left(1, 2);
	const spk::Vector2Int right(4, 6);

	EXPECT_FLOAT_EQ(left.squaredDistance(right), 25.0f);
}

TEST(Vector2GeometryTest, DistanceWorks)
{
	const spk::Vector2Int left(1, 2);
	const spk::Vector2Int right(4, 6);

	EXPECT_FLOAT_EQ(left.distance(right), 5.0f);
}

TEST(Vector2GeometryTest, DotProductWorks)
{
	const spk::Vector2Int left(1, 2);
	const spk::Vector2Int right(3, 4);

	EXPECT_FLOAT_EQ(left.dot(right), 11.0f);
}

TEST(Vector2GeometryTest, CrossProductWorks)
{
	const spk::Vector2Int left(1, 2);
	const spk::Vector2Int right(3, 4);

	EXPECT_FLOAT_EQ(left.cross(right), -2.0f);
}

TEST(Vector2GeometryTest, PerpendicularWorks)
{
	const spk::Vector2Int value(2, 3);
	const spk::Vector2Int result = value.perpendicular();

	EXPECT_EQ(result.x, -3);
	EXPECT_EQ(result.y, 2);
}

TEST(Vector2InterpolationTest, LerpAtZeroReturnsStart)
{
	const spk::Vector2 start(0.0f, 0.0f);
	const spk::Vector2 end(10.0f, 20.0f);

	const spk::Vector2 result = spk::Vector2::lerp(start, end, 0.0f);

	EXPECT_FLOAT_EQ(result.x, 0.0f);
	EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(Vector2InterpolationTest, LerpAtOneReturnsEnd)
{
	const spk::Vector2 start(0.0f, 0.0f);
	const spk::Vector2 end(10.0f, 20.0f);

	const spk::Vector2 result = spk::Vector2::lerp(start, end, 1.0f);

	EXPECT_FLOAT_EQ(result.x, 10.0f);
	EXPECT_FLOAT_EQ(result.y, 20.0f);
}

TEST(Vector2InterpolationTest, LerpAtHalfReturnsMiddle)
{
	const spk::Vector2 start(0.0f, 0.0f);
	const spk::Vector2 end(10.0f, 20.0f);

	const spk::Vector2 result = spk::Vector2::lerp(start, end, 0.5f);

	EXPECT_FLOAT_EQ(result.x, 5.0f);
	EXPECT_FLOAT_EQ(result.y, 10.0f);
}

TEST(Vector2MinMaxTest, MinOfTwoVectorsWorks)
{
	const spk::Vector2Int left(5, 1);
	const spk::Vector2Int right(3, 4);

	const spk::Vector2Int result = spk::Vector2Int::min(left, right);

	EXPECT_EQ(result.x, 3);
	EXPECT_EQ(result.y, 1);
}

TEST(Vector2MinMaxTest, MaxOfTwoVectorsWorks)
{
	const spk::Vector2Int left(5, 1);
	const spk::Vector2Int right(3, 4);

	const spk::Vector2Int result = spk::Vector2Int::max(left, right);

	EXPECT_EQ(result.x, 5);
	EXPECT_EQ(result.y, 4);
}

TEST(Vector2MinMaxTest, VariadicMinWorks)
{
	const spk::Vector2Int result = spk::Vector2Int::min(
		spk::Vector2Int(5, 9),
		spk::Vector2Int(3, 4),
		spk::Vector2Int(7, 1),
		spk::Vector2Int(6, 8));

	EXPECT_EQ(result.x, 3);
	EXPECT_EQ(result.y, 1);
}

TEST(Vector2MinMaxTest, VariadicMaxWorks)
{
	const spk::Vector2Int result = spk::Vector2Int::max(
		spk::Vector2Int(5, 9),
		spk::Vector2Int(3, 4),
		spk::Vector2Int(7, 1),
		spk::Vector2Int(6, 8));

	EXPECT_EQ(result.x, 7);
	EXPECT_EQ(result.y, 9);
}

TEST(Vector2MinMaxTest, InitializerListMinWorks)
{
	const spk::Vector2Int result = spk::Vector2Int::min({
		spk::Vector2Int(5, 9),
		spk::Vector2Int(3, 4),
		spk::Vector2Int(7, 1),
		spk::Vector2Int(6, 8)
	});

	EXPECT_EQ(result.x, 3);
	EXPECT_EQ(result.y, 1);
}

TEST(Vector2MinMaxTest, InitializerListMinThrowsWhenEmpty)
{
	EXPECT_THROW(static_cast<void>(spk::Vector2Int::min({})), std::invalid_argument);
}

TEST(Vector2MinMaxTest, InitializerListMaxWorks)
{
	const spk::Vector2Int result = spk::Vector2Int::max({
		spk::Vector2Int(5, 9),
		spk::Vector2Int(3, 4),
		spk::Vector2Int(7, 1),
		spk::Vector2Int(6, 8)
	});

	EXPECT_EQ(result.x, 7);
	EXPECT_EQ(result.y, 9);
}

TEST(Vector2MinMaxTest, InitializerListMaxThrowsWhenEmpty)
{
	EXPECT_THROW(static_cast<void>(spk::Vector2Int::max({})), std::invalid_argument);
}

TEST(Vector2ClampTest, ClampWorksWithOrderedBounds)
{
	const spk::Vector2Int value(5, -2);
	const spk::Vector2Int result = spk::Vector2Int::clamp(
		value,
		spk::Vector2Int(-1, 0),
		spk::Vector2Int(2, 1));

	EXPECT_EQ(result.x, 2);
	EXPECT_EQ(result.y, 0);
}

TEST(Vector2ClampTest, ClampWorksWithUnorderedBounds)
{
	const spk::Vector2Int value(5, -2);
	const spk::Vector2Int result = spk::Vector2Int::clamp(
		value,
		spk::Vector2Int(2, 1),
		spk::Vector2Int(-1, 0));

	EXPECT_EQ(result.x, 2);
	EXPECT_EQ(result.y, 0);
}

TEST(Vector2ClampTest, ClampedCallsStaticClampBehavior)
{
	const spk::Vector2Int value(5, -2);
	const spk::Vector2Int result = value.clamped(
		spk::Vector2Int(-1, 0),
		spk::Vector2Int(2, 1));

	EXPECT_EQ(result.x, 2);
	EXPECT_EQ(result.y, 0);
}

TEST(Vector2BetweenTest, IsBetweenReturnsTrueInsideOrderedBounds)
{
	EXPECT_TRUE(spk::Vector2Int::isBetween(
		spk::Vector2Int(5, 5),
		spk::Vector2Int(0, 0),
		spk::Vector2Int(10, 10)));
}

TEST(Vector2BetweenTest, IsBetweenReturnsFalseOutsideOrderedBounds)
{
	EXPECT_FALSE(spk::Vector2Int::isBetween(
		spk::Vector2Int(11, 5),
		spk::Vector2Int(0, 0),
		spk::Vector2Int(10, 10)));
}

TEST(Vector2BetweenTest, IsBetweenHandlesUnorderedBounds)
{
	EXPECT_TRUE(spk::Vector2Int::isBetween(
		spk::Vector2Int(5, 5),
		spk::Vector2Int(10, 10),
		spk::Vector2Int(0, 0)));
}

TEST(Vector2ConversionTest, ExplicitConversionOperatorWorks)
{
	const spk::Vector2Int source(3, -4);
	const spk::IVector2<float> converted = static_cast<spk::IVector2<float>>(source);

	EXPECT_FLOAT_EQ(converted.x, 3.0f);
	EXPECT_FLOAT_EQ(converted.y, -4.0f);
}

TEST(Vector2ConversionTest, AliasTypesMatchExpectedUnderlyingTypes)
{
	static_assert(std::is_same_v<spk::Vector2, spk::IVector2<float_t>>);
	static_assert(std::is_same_v<spk::Vector2Int, spk::IVector2<std::int32_t>>);
	static_assert(std::is_same_v<spk::Vector2UInt, spk::IVector2<std::uint32_t>>);
}