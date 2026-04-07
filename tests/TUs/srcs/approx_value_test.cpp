#include <gtest/gtest.h>

#include <type_traits>

#include "spk_approx_value.hpp"
#include "spk_constants.hpp"

namespace
{
	template <typename TType>
	constexpr TType defaultPrecision()
	{
		return static_cast<TType>(spk::Math::Constants::pointPrecision);
	}
}

TEST(ApproxValueIntegerTest, DefaultConstructionInitializesToZero)
{
	const spk::ApproxValue<int> value;

	EXPECT_EQ(value.value, 0);
}

TEST(ApproxValueIntegerTest, ExplicitConstructionStoresExactValue)
{
	const spk::ApproxValue<int> value(42);

	EXPECT_EQ(value.value, 42);
}

TEST(ApproxValueIntegerTest, IntegerEqualsSameInteger)
{
	const spk::ApproxValue<int> value(42);

	EXPECT_TRUE(value == 42);
	EXPECT_TRUE(42 == value);
	EXPECT_FALSE(value != 42);
	EXPECT_FALSE(42 != value);
}

TEST(ApproxValueIntegerTest, IntegerDoesNotEqualDifferentInteger)
{
	const spk::ApproxValue<int> value(42);

	EXPECT_FALSE(value == 41);
	EXPECT_FALSE(41 == value);
	EXPECT_TRUE(value != 41);
	EXPECT_TRUE(41 != value);
}

TEST(ApproxValueIntegerTest, IntegerWrapperEqualsIntegerWrapperWithSameValue)
{
	const spk::ApproxValue<int> left(42);
	const spk::ApproxValue<int> right(42);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);
}

TEST(ApproxValueIntegerTest, IntegerWrapperDoesNotEqualIntegerWrapperWithDifferentValue)
{
	const spk::ApproxValue<int> left(42);
	const spk::ApproxValue<int> right(43);

	EXPECT_FALSE(left == right);
	EXPECT_TRUE(left != right);
}

TEST(ApproxValueIntegerTest, IntegerComparedToExactFloatingValueIsEqual)
{
	const spk::ApproxValue<int> value(10);

	EXPECT_TRUE(value == 10.0f);
	EXPECT_TRUE(10.0f == value);
	EXPECT_FALSE(value != 10.0f);
	EXPECT_FALSE(10.0f != value);
}

TEST(ApproxValueIntegerTest, IntegerComparedToFloatingValueWithinDefaultPrecisionIsEqual)
{
	const spk::ApproxValue<int> value(10);

	const float epsilon = defaultPrecision<float>();
	const float nearbyValue = 10.0f + (epsilon * 0.5f);

	EXPECT_TRUE(value == nearbyValue);
	EXPECT_TRUE(nearbyValue == value);
	EXPECT_FALSE(value != nearbyValue);
	EXPECT_FALSE(nearbyValue != value);
}

TEST(ApproxValueIntegerTest, IntegerComparedToFloatingValueOutsideDefaultPrecisionIsNotEqual)
{
	const spk::ApproxValue<int> value(10);

	const float epsilon = defaultPrecision<float>();
	const float farValue = 10.0f + (epsilon * 2.0f);

	EXPECT_FALSE(value == farValue);
	EXPECT_FALSE(farValue == value);
	EXPECT_TRUE(value != farValue);
	EXPECT_TRUE(farValue != value);
}

TEST(ApproxValueIntegerTest, IntegerAndFloatingWrapperWithinDefaultPrecisionAreEqual)
{
	const spk::ApproxValue<int> left(10);
	const spk::ApproxValue<float> right(10.0f + (defaultPrecision<float>() * 0.5f));

	EXPECT_TRUE(left == right);
	EXPECT_TRUE(right == left);
	EXPECT_FALSE(left != right);
	EXPECT_FALSE(right != left);
}

TEST(ApproxValueIntegerTest, IntegerAndFloatingWrapperOutsideDefaultPrecisionAreNotEqual)
{
	const spk::ApproxValue<int> left(10);
	const spk::ApproxValue<float> right(10.0f + (defaultPrecision<float>() * 2.0f));

	EXPECT_FALSE(left == right);
	EXPECT_FALSE(right == left);
	EXPECT_TRUE(left != right);
	EXPECT_TRUE(right != left);
}

TEST(ApproxValueFloatTest, DefaultConstructionInitializesValueAndDefaultEpsilon)
{
	const spk::ApproxValue<float> value;

	EXPECT_FLOAT_EQ(value.value, 0.0f);
	EXPECT_FLOAT_EQ(value.epsilon, defaultPrecision<float>());
}

TEST(ApproxValueFloatTest, ExplicitConstructionStoresValueAndDefaultEpsilon)
{
	const spk::ApproxValue<float> value(1.5f);

	EXPECT_FLOAT_EQ(value.value, 1.5f);
	EXPECT_FLOAT_EQ(value.epsilon, defaultPrecision<float>());
}

TEST(ApproxValueFloatTest, ExplicitConstructionWithCustomEpsilonStoresBoth)
{
	const spk::ApproxValue<float> value(1.5f, 0.25f);

	EXPECT_FLOAT_EQ(value.value, 1.5f);
	EXPECT_FLOAT_EQ(value.epsilon, 0.25f);
}

TEST(ApproxValueFloatTest, FloatEqualsSameFloat)
{
	const spk::ApproxValue<float> value(1.0f);

	EXPECT_TRUE(value == 1.0f);
	EXPECT_TRUE(1.0f == value);
	EXPECT_FALSE(value != 1.0f);
	EXPECT_FALSE(1.0f != value);
}

TEST(ApproxValueFloatTest, FloatWithinDefaultEpsilonIsEqual)
{
	const float epsilon = defaultPrecision<float>();
	const spk::ApproxValue<float> value(1.0f);

	EXPECT_TRUE(value == (1.0f + epsilon * 0.5f));
	EXPECT_TRUE((1.0f + epsilon * 0.5f) == value);
}

TEST(ApproxValueFloatTest, FloatOutsideDefaultEpsilonIsNotEqual)
{
	const float epsilon = defaultPrecision<float>();
	const spk::ApproxValue<float> value(1.0f);

	EXPECT_FALSE(value == (1.0f + epsilon * 2.0f));
	EXPECT_FALSE((1.0f + epsilon * 2.0f) == value);

	EXPECT_TRUE(value != (1.0f + epsilon * 2.0f));
	EXPECT_TRUE((1.0f + epsilon * 2.0f) != value);
}

TEST(ApproxValueFloatTest, FloatAtExactPositiveEpsilonBoundaryIsDifferent)
{
	const float epsilon = defaultPrecision<float>();
	const spk::ApproxValue<float> value(5.0f);

	EXPECT_TRUE(value != (5.0f + epsilon));
	EXPECT_TRUE((5.0f + epsilon) != value);
}

TEST(ApproxValueFloatTest, FloatAtExactNegativeEpsilonBoundaryIsDifferent)
{
	const float epsilon = defaultPrecision<float>();
	const spk::ApproxValue<float> value(5.0f);

	EXPECT_TRUE(value != (5.0f - epsilon));
	EXPECT_TRUE((5.0f - epsilon) != value);
}

TEST(ApproxValueFloatTest, FloatJustPastEpsilonBoundaryIsNotEqual)
{
	const float epsilon = defaultPrecision<float>();
	const spk::ApproxValue<float> value(5.0f);

	EXPECT_FALSE(value == (5.0f + epsilon * 1.001f));
	EXPECT_FALSE((5.0f + epsilon * 1.001f) == value);
}

TEST(ApproxValueFloatTest, FloatWrapperEqualsFloatWrapperWithinLeftCustomEpsilon)
{
	const spk::ApproxValue<float> left(10.0f, 0.5f);
	const spk::ApproxValue<float> right(10.4f, 1.0f);

	EXPECT_TRUE(left == right);
	EXPECT_TRUE(right == left);
}

TEST(ApproxValueFloatTest, FloatWrapperUsesMinimumEpsilonWhenBothAreFloating)
{
	const spk::ApproxValue<float> left(10.0f, 0.5f);
	const spk::ApproxValue<float> right(10.6f, 1.0f);

	EXPECT_FALSE(left == right);
	EXPECT_FALSE(right == left);
	EXPECT_TRUE(left != right);
	EXPECT_TRUE(right != left);
}

TEST(ApproxValueFloatTest, FloatWrapperWithSameValueButDifferentEpsilonStillEquals)
{
	const spk::ApproxValue<float> left(3.0f, 0.001f);
	const spk::ApproxValue<float> right(3.0f, 100.0f);

	EXPECT_TRUE(left == right);
	EXPECT_FALSE(left != right);
}

TEST(ApproxValueFloatTest, FloatComparedToIntegerEqualWhenNumericallySame)
{
	const spk::ApproxValue<float> value(8.0f);

	EXPECT_TRUE(value == 8);
	EXPECT_TRUE(8 == value);
	EXPECT_FALSE(value != 8);
	EXPECT_FALSE(8 != value);
}

TEST(ApproxValueFloatTest, FloatComparedToIntegerWithinCustomEpsilonIsEqual)
{
	const spk::ApproxValue<float> value(8.2f, 0.25f);

	EXPECT_TRUE(value == 8);
	EXPECT_TRUE(8 == value);
}

TEST(ApproxValueFloatTest, FloatComparedToIntegerOutsideCustomEpsilonIsNotEqual)
{
	const spk::ApproxValue<float> value(8.3f, 0.25f);

	EXPECT_FALSE(value == 8);
	EXPECT_FALSE(8 == value);

	EXPECT_TRUE(value != 8);
	EXPECT_TRUE(8 != value);
}

TEST(ApproxValueFloatTest, NegativeValuesWithinEpsilonAreEqual)
{
	const spk::ApproxValue<double> value(-2.0, 0.01);

	EXPECT_TRUE(value == -2.005);
	EXPECT_TRUE(-2.005 == value);
	EXPECT_FALSE(value != -2.005);
}

TEST(ApproxValueFloatTest, NegativeValuesOutsideEpsilonAreNotEqual)
{
	const spk::ApproxValue<double> value(-2.0, 0.01);

	EXPECT_FALSE(value == -2.02);
	EXPECT_FALSE(-2.02 == value);
	EXPECT_TRUE(value != -2.02);
}

TEST(ApproxValueFloatTest, ZeroComparedWithinEpsilonIsEqual)
{
	const spk::ApproxValue<float> value(0.0f, 0.01f);

	EXPECT_TRUE(value == 0.005f);
	EXPECT_TRUE(0.005f == value);
}

TEST(ApproxValueFloatTest, ZeroComparedOutsideEpsilonIsNotEqual)
{
	const spk::ApproxValue<float> value(0.0f, 0.01f);

	EXPECT_FALSE(value == 0.02f);
	EXPECT_FALSE(0.02f == value);
}

TEST(ApproxValueDeductionGuideTest, DeductionGuideWithIntegerProducesIntegerWrapper)
{
	const spk::ApproxValue value(12);

	static_assert(std::is_same_v<decltype(value), const spk::ApproxValue<int>>);
	EXPECT_EQ(value.value, 12);
}

TEST(ApproxValueDeductionGuideTest, DeductionGuideWithFloatProducesFloatWrapper)
{
	const spk::ApproxValue value(1.25f);

	static_assert(std::is_same_v<decltype(value), const spk::ApproxValue<float>>);
	EXPECT_FLOAT_EQ(value.value, 1.25f);
	EXPECT_FLOAT_EQ(value.epsilon, defaultPrecision<float>());
}

TEST(ApproxValueDeductionGuideTest, DeductionGuideWithFloatAndCustomEpsilonProducesFloatWrapper)
{
	const spk::ApproxValue value(1.25f, 0.5f);

	static_assert(std::is_same_v<decltype(value), const spk::ApproxValue<float>>);
	EXPECT_FLOAT_EQ(value.value, 1.25f);
	EXPECT_FLOAT_EQ(value.epsilon, 0.5f);
}

TEST(ApproxValueMixedWrapperTest, IntegerWrapperAndDoubleWrapperExactSameValueAreEqual)
{
	const spk::ApproxValue<int> left(7);
	const spk::ApproxValue<double> right(7.0);

	EXPECT_TRUE(left == right);
	EXPECT_TRUE(right == left);
	EXPECT_FALSE(left != right);
	EXPECT_FALSE(right != left);
}

TEST(ApproxValueMixedWrapperTest, IntegerWrapperAndDoubleWrapperWithinRightEpsilonAreEqual)
{
	const spk::ApproxValue<int> left(7);
	const spk::ApproxValue<double> right(7.0 + defaultPrecision<double>() * 0.5);

	EXPECT_TRUE(left == right);
	EXPECT_TRUE(right == left);
}

TEST(ApproxValueMixedWrapperTest, IntegerWrapperAndDoubleWrapperOutsideRightEpsilonAreNotEqual)
{
	const spk::ApproxValue<int> left(7);
	const spk::ApproxValue<double> right(7.0 + defaultPrecision<double>() * 2.0);

	EXPECT_FALSE(left == right);
	EXPECT_FALSE(right == left);
	EXPECT_TRUE(left != right);
	EXPECT_TRUE(right != left);
}

TEST(ApproxValueMixedWrapperTest, TwoFloatingWrappersUseSmallerEpsilon)
{
	const spk::ApproxValue<double> left(100.0, 0.1);
	const spk::ApproxValue<float> right(100.15f, 0.2f);

	EXPECT_FALSE(left == right);
	EXPECT_FALSE(right == left);
}

TEST(ApproxValueMixedWrapperTest, TwoFloatingWrappersEqualAtSmallerEpsilonBoundary)
{
	const spk::ApproxValue<double> left(100.0, 0.1);
	const spk::ApproxValue<float> right(100.1f, 0.2f);

	EXPECT_TRUE(left == right);
	EXPECT_TRUE(right == left);
}