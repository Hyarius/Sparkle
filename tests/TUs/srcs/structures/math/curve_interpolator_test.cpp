#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

#include "structures/math/spk_curve_interpolator.hpp"
#include "structures/math/spk_vector2.hpp"

namespace
{
	constexpr float NOT_A_NUMBER = std::numeric_limits<float>::quiet_NaN();
	constexpr float INFINITY_VALUE = std::numeric_limits<float>::infinity();

	using Point = spk::CurveInterpolator::Point;

	spk::CurveInterpolator makeCurve(
		std::initializer_list<Point> p_points,
		spk::CurveInterpolator::Interpolation p_interpolation =
			spk::CurveInterpolator::Interpolation::MonotoneCubicHermite,
		spk::CurveInterpolator::Extrapolation p_extrapolation =
			spk::CurveInterpolator::Extrapolation::Linear)
	{
		spk::CurveInterpolator curve;
		curve.assign(p_points);
		curve.setInterpolation(p_interpolation);
		curve.setExtrapolation(p_extrapolation);

		return curve;
	}
}

TEST(CurveInterpolatorTest, DefaultsAreMonotoneAndLinear)
{
	const spk::CurveInterpolator curve;

	EXPECT_TRUE(curve.empty());
	EXPECT_EQ(curve.nbPoints(), 0u);
	EXPECT_EQ(curve.interpolation(), spk::CurveInterpolator::Interpolation::MonotoneCubicHermite);
	EXPECT_EQ(curve.extrapolation(), spk::CurveInterpolator::Extrapolation::Linear);
}

TEST(CurveInterpolatorTest, InsertScalarStoresPoint)
{
	spk::CurveInterpolator curve;

	curve.insert(1.0f, 2.0f);

	EXPECT_EQ(curve.nbPoints(), 1u);
	EXPECT_FALSE(curve.empty());
	EXPECT_FLOAT_EQ(curve.points().at(1.0f), 2.0f);
}

TEST(CurveInterpolatorTest, InsertPointStoresPoint)
{
	spk::CurveInterpolator curve;

	curve.insert(Point(3.0f, 4.0f));

	EXPECT_FLOAT_EQ(curve.points().at(3.0f), 4.0f);
}

TEST(CurveInterpolatorTest, InsertSpanStoresEveryPoint)
{
	const std::vector<Point> points = {Point(0.0f, 0.0f), Point(1.0f, 1.0f), Point(2.0f, 4.0f)};

	spk::CurveInterpolator curve;
	curve.insert(std::span<const Point>(points));

	EXPECT_EQ(curve.nbPoints(), 3u);
}

TEST(CurveInterpolatorTest, InsertOverwritesExistingX)
{
	spk::CurveInterpolator curve;

	curve.insert(1.0f, 2.0f);
	curve.insert(1.0f, 9.0f);

	EXPECT_EQ(curve.nbPoints(), 1u);
	EXPECT_FLOAT_EQ(curve.points().at(1.0f), 9.0f);
}

TEST(CurveInterpolatorTest, AssignSpanReplacesContents)
{
	spk::CurveInterpolator curve;
	curve.insert(10.0f, 10.0f);

	const std::vector<Point> points = {Point(0.0f, 0.0f), Point(1.0f, 1.0f)};
	curve.assign(std::span<const Point>(points));

	EXPECT_EQ(curve.nbPoints(), 2u);
	EXPECT_EQ(curve.points().count(10.0f), 0u);
}

TEST(CurveInterpolatorTest, AssignInitializerListReplacesContents)
{
	spk::CurveInterpolator curve;
	curve.insert(10.0f, 10.0f);

	curve.assign({Point(0.0f, 0.0f), Point(2.0f, 2.0f)});

	EXPECT_EQ(curve.nbPoints(), 2u);
}

TEST(CurveInterpolatorTest, RemoveReturnsWhetherPointExisted)
{
	spk::CurveInterpolator curve;
	curve.insert(1.0f, 1.0f);

	EXPECT_TRUE(curve.remove(1.0f));
	EXPECT_FALSE(curve.remove(1.0f));
	EXPECT_TRUE(curve.empty());
}

TEST(CurveInterpolatorTest, ClearEmptiesCurve)
{
	spk::CurveInterpolator curve;
	curve.insert(1.0f, 1.0f);
	curve.insert(2.0f, 2.0f);

	curve.clear();

	EXPECT_TRUE(curve.empty());
	EXPECT_EQ(curve.nbPoints(), 0u);
}

TEST(CurveInterpolatorTest, GettersAndSettersRoundTrip)
{
	spk::CurveInterpolator curve;

	curve.setInterpolation(spk::CurveInterpolator::Interpolation::Bezier);
	curve.setExtrapolation(spk::CurveInterpolator::Extrapolation::Clamp);

	EXPECT_EQ(curve.interpolation(), spk::CurveInterpolator::Interpolation::Bezier);
	EXPECT_EQ(curve.extrapolation(), spk::CurveInterpolator::Extrapolation::Clamp);
}

TEST(CurveInterpolatorTest, InsertRejectsNonFiniteValues)
{
	spk::CurveInterpolator curve;

	EXPECT_THROW(curve.insert(NOT_A_NUMBER, 0.0f), std::invalid_argument);
	EXPECT_THROW(curve.insert(0.0f, INFINITY_VALUE), std::invalid_argument);
	EXPECT_THROW(curve.insert(Point(NOT_A_NUMBER, 0.0f)), std::invalid_argument);
}

TEST(CurveInterpolatorTest, AssignRejectsNonFiniteValues)
{
	spk::CurveInterpolator curve;

	const std::vector<Point> badSpan = {Point(0.0f, 0.0f), Point(NOT_A_NUMBER, 1.0f)};
	EXPECT_THROW(curve.assign(std::span<const Point>(badSpan)), std::invalid_argument);
	EXPECT_THROW(curve.assign({Point(0.0f, NOT_A_NUMBER)}), std::invalid_argument);
}

TEST(CurveInterpolatorTest, RemoveRejectsNonFiniteX)
{
	spk::CurveInterpolator curve;

	EXPECT_THROW(curve.remove(NOT_A_NUMBER), std::invalid_argument);
}

TEST(CurveInterpolatorTest, EvaluateRejectsNonFiniteX)
{
	spk::CurveInterpolator curve;
	curve.insert(0.0f, 0.0f);
	curve.insert(1.0f, 1.0f);

	EXPECT_THROW(auto value = curve.evaluate(NOT_A_NUMBER), std::invalid_argument);
}

TEST(CurveInterpolatorTest, EvaluateEmptyCurveReturnsZero)
{
	const spk::CurveInterpolator curve;

	EXPECT_FLOAT_EQ(curve.evaluate(5.0f), 0.0f);
}

TEST(CurveInterpolatorTest, EvaluateSinglePointReturnsThatValue)
{
	spk::CurveInterpolator curve;
	curve.insert(2.0f, 7.0f);

	EXPECT_FLOAT_EQ(curve.evaluate(-100.0f), 7.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(2.0f), 7.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(100.0f), 7.0f);
}

TEST(CurveInterpolatorTest, EvaluateReturnsExactNodeValues)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 2.0f),
		Point(2.0f, 1.0f)});

	EXPECT_FLOAT_EQ(curve.evaluate(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(1.0f), 2.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(2.0f), 1.0f);
}

TEST(CurveInterpolatorTest, CallAndSubscriptOperatorsForwardToEvaluate)
{
	const spk::CurveInterpolator curve = makeCurve({Point(0.0f, 0.0f), Point(1.0f, 1.0f)});

	EXPECT_FLOAT_EQ(curve(0.5f), curve.evaluate(0.5f));
	EXPECT_FLOAT_EQ(curve[0.5f], curve.evaluate(0.5f));
}

// ---------------------------------------------------------------------------
// Interpolation modes
// ---------------------------------------------------------------------------

class CurveInterpolatorModeTest : public ::testing::TestWithParam<spk::CurveInterpolator::Interpolation>
{
};

TEST_P(CurveInterpolatorModeTest, InteriorEvaluationStaysWithinDataEnvelopeForMonotoneData)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, 2.0f),
		Point(3.0f, 4.0f),
		Point(4.0f, 8.0f)},
		GetParam());

	for (int i = 0; i <= 40; ++i)
	{
		const float x = static_cast<float>(i) * 0.1f;
		const float y = curve.evaluate(x);

		EXPECT_TRUE(std::isfinite(y));
		EXPECT_GE(y, -1.0f);
		EXPECT_LE(y, 9.0f);
	}
}

TEST_P(CurveInterpolatorModeTest, TwoPointCurveIsHandled)
{
	const spk::CurveInterpolator curve = makeCurve({Point(0.0f, 1.0f), Point(2.0f, 5.0f)}, GetParam());

	EXPECT_FLOAT_EQ(curve.evaluate(0.0f), 1.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(2.0f), 5.0f);
	EXPECT_TRUE(std::isfinite(curve.evaluate(1.0f)));
}

TEST_P(CurveInterpolatorModeTest, NonMonotoneDataIsHandled)
{
	// Rising, peak, falling, flat, falling: exercises sign-change handling.
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 3.0f),
		Point(2.0f, 1.0f),
		Point(3.0f, 1.0f),
		Point(4.0f, -2.0f)},
		GetParam());

	for (int i = 0; i <= 40; ++i)
	{
		EXPECT_TRUE(std::isfinite(curve.evaluate(static_cast<float>(i) * 0.1f)));
	}
}

TEST_P(CurveInterpolatorModeTest, LinearExtrapolationProducesFiniteValuesOutsideDomain)
{
	spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, 3.0f)},
		GetParam(),
		spk::CurveInterpolator::Extrapolation::Linear);

	EXPECT_TRUE(std::isfinite(curve.evaluate(-5.0f)));
	EXPECT_TRUE(std::isfinite(curve.evaluate(10.0f)));
}

TEST_P(CurveInterpolatorModeTest, LinearExtrapolationOnTwoPointCurve)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 2.0f)},
		GetParam(),
		spk::CurveInterpolator::Extrapolation::Linear);

	EXPECT_TRUE(std::isfinite(curve.evaluate(-3.0f)));
	EXPECT_TRUE(std::isfinite(curve.evaluate(5.0f)));
}

INSTANTIATE_TEST_SUITE_P(
	AllModes,
	CurveInterpolatorModeTest,
	::testing::Values(
		spk::CurveInterpolator::Interpolation::Linear,
		spk::CurveInterpolator::Interpolation::Cubic,
		spk::CurveInterpolator::Interpolation::MonotoneCubicHermite,
		spk::CurveInterpolator::Interpolation::Bezier));

// ---------------------------------------------------------------------------
// Exactness checks for specific modes
// ---------------------------------------------------------------------------

TEST(CurveInterpolatorTest, LinearInterpolationIsExactBetweenNodes)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(2.0f, 4.0f)},
		spk::CurveInterpolator::Interpolation::Linear);

	EXPECT_NEAR(curve.evaluate(1.0f), 2.0f, 1e-5f);
	EXPECT_NEAR(curve.evaluate(0.5f), 1.0f, 1e-5f);
}

TEST(CurveInterpolatorTest, LinearExtrapolationFollowsEndSlope)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, 2.0f)},
		spk::CurveInterpolator::Interpolation::Linear,
		spk::CurveInterpolator::Extrapolation::Linear);

	// Slope of 1 on both ends.
	EXPECT_NEAR(curve.evaluate(-2.0f), -2.0f, 1e-4f);
	EXPECT_NEAR(curve.evaluate(4.0f), 4.0f, 1e-4f);
}

TEST(CurveInterpolatorTest, MonotoneCubicPreservesMonotonicity)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 0.0f),
		Point(2.0f, 1.0f),
		Point(3.0f, 1.0f)},
		spk::CurveInterpolator::Interpolation::MonotoneCubicHermite);

	float previous = curve.evaluate(0.0f);

	for (int i = 1; i <= 30; ++i)
	{
		const float current = curve.evaluate(static_cast<float>(i) * 0.1f);

		EXPECT_GE(current, previous - 1e-4f);
		previous = current;
	}
}

// ---------------------------------------------------------------------------
// Extrapolation policies
// ---------------------------------------------------------------------------

TEST(CurveInterpolatorTest, ClampExtrapolationReturnsEndpointValues)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 1.0f),
		Point(1.0f, 2.0f),
		Point(2.0f, 5.0f)},
		spk::CurveInterpolator::Interpolation::Linear,
		spk::CurveInterpolator::Extrapolation::Clamp);

	EXPECT_FLOAT_EQ(curve.evaluate(-10.0f), 1.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(10.0f), 5.0f);
}

TEST(CurveInterpolatorTest, ThrowExtrapolationThrowsOutsideDomain)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f)},
		spk::CurveInterpolator::Interpolation::Linear,
		spk::CurveInterpolator::Extrapolation::Throw);

	EXPECT_THROW(auto value = curve.evaluate(-1.0f), std::out_of_range);
	EXPECT_THROW(auto value = curve.evaluate(2.0f), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Monotone tangent boundary branches
// ---------------------------------------------------------------------------

TEST(CurveInterpolatorTest, MonotoneBoundaryClampsOvershootToZero)
{
	// Accelerating data: the one-sided endpoint tangent formula overshoots with
	// the opposite sign of the first secant, so it is clamped to zero.
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, 5.0f)},
		spk::CurveInterpolator::Interpolation::MonotoneCubicHermite);

	for (int i = 0; i <= 20; ++i)
	{
		EXPECT_TRUE(std::isfinite(curve.evaluate(static_cast<float>(i) * 0.1f)));
	}
}

TEST(CurveInterpolatorTest, MonotoneBoundaryLimitsTangentToTripleSecant)
{
	// A sharp peak then deep drop forces the boundary tangent above 3x the first
	// secant while the two secants differ in sign, hitting the 3*d clamp branch.
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, -3.0f)},
		spk::CurveInterpolator::Interpolation::MonotoneCubicHermite);

	for (int i = 0; i <= 20; ++i)
	{
		EXPECT_TRUE(std::isfinite(curve.evaluate(static_cast<float>(i) * 0.1f)));
	}
}

TEST(CurveInterpolatorTest, MonotoneLastBoundaryLimitsTangentToTripleSecant)
{
	// Mirror of the first-boundary case: a steep drop into the final segment
	// makes the last endpoint tangent exceed 3x the final secant while the two
	// secants differ in sign, hitting the 3*d clamp branch of the last tangent.
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 4.0f),
		Point(1.0f, 0.0f),
		Point(2.0f, 1.0f)},
		spk::CurveInterpolator::Interpolation::MonotoneCubicHermite);

	// Evaluate inside the last segment so the right endpoint is the last point.
	for (int i = 10; i <= 20; ++i)
	{
		EXPECT_TRUE(std::isfinite(curve.evaluate(static_cast<float>(i) * 0.1f)));
	}
}

TEST(CurveInterpolatorTest, MonotoneFlatSegmentProducesZeroInteriorTangent)
{
	// Equal consecutive y values make a secant zero, exercising the flat branch
	// of the interior monotone tangent.
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, 1.0f),
		Point(3.0f, 2.0f)},
		spk::CurveInterpolator::Interpolation::MonotoneCubicHermite);

	for (int i = 0; i <= 30; ++i)
	{
		EXPECT_TRUE(std::isfinite(curve.evaluate(static_cast<float>(i) * 0.1f)));
	}
}

// ---------------------------------------------------------------------------
// Bezier interior tangent uses central differences with >= 3 points
// ---------------------------------------------------------------------------

TEST(CurveInterpolatorTest, BezierInteriorSegmentIsFinite)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 2.0f),
		Point(2.0f, 1.0f),
		Point(3.0f, 3.0f)},
		spk::CurveInterpolator::Interpolation::Bezier);

	// Evaluate inside the middle segment so both endpoints use the interior
	// (central-difference) Catmull-Rom tangent.
	EXPECT_TRUE(std::isfinite(curve.evaluate(1.5f)));
	EXPECT_FLOAT_EQ(curve.evaluate(1.0f), 2.0f);
	EXPECT_FLOAT_EQ(curve.evaluate(2.0f), 1.0f);
}

TEST(CurveInterpolatorTest, CubicInteriorSegmentIsFiniteWithManyPoints)
{
	const spk::CurveInterpolator curve = makeCurve({
		Point(0.0f, 0.0f),
		Point(1.0f, 1.0f),
		Point(2.0f, 0.0f),
		Point(3.0f, 1.0f),
		Point(4.0f, 0.0f)},
		spk::CurveInterpolator::Interpolation::Cubic);

	for (int i = 0; i <= 40; ++i)
	{
		EXPECT_TRUE(std::isfinite(curve.evaluate(static_cast<float>(i) * 0.1f)));
	}
}
