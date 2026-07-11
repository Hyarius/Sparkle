#include <gtest/gtest.h>

#include "structures/graphics/geometry/spk_polygon_2d.hpp"
#include "structures/math/spk_vector2.hpp"

#include <initializer_list>
#include <stdexcept>
#include <vector>

namespace
{
	using Polygon = spk::Polygon2D<spk::Vector2>;

	constexpr float ComparisonTolerance = 0.0001f;

	[[nodiscard]] spk::Vector2 interpolatePayload(
		const spk::Vector2 &p_from,
		const spk::Vector2 &p_to,
		float p_interpolation) noexcept
	{
		return p_from + (p_to - p_from) * p_interpolation;
	}

	[[nodiscard]] Polygon makePolygon(std::initializer_list<spk::Vector2> p_positions)
	{
		Polygon::Builder builder;
		builder.reserve(p_positions.size());
		for (const spk::Vector2 &position : p_positions)
		{
			builder.addVertex({.position = position, .data = position});
		}
		return std::move(builder).bake();
	}

	[[nodiscard]] Polygon makeUnitSquare(bool p_clockwise)
	{
		if (p_clockwise)
		{
			return makePolygon({
				{0.0f, 0.0f},
				{0.0f, 1.0f},
				{1.0f, 1.0f},
				{1.0f, 0.0f}});
		}

		return makePolygon({
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 1.0f},
			{0.0f, 1.0f}});
	}

	[[nodiscard]] float totalArea(const std::vector<Polygon> &p_polygons)
	{
		float result = 0.0f;
		for (const Polygon &polygon : p_polygons)
		{
			result += polygon.area();
		}
		return result;
	}

	void expectPayloadMatchesPosition(const std::vector<Polygon> &p_polygons)
	{
		for (const Polygon &polygon : p_polygons)
		{
			for (const Polygon::Vertex &vertex : polygon)
			{
				EXPECT_NEAR(vertex.data.x, vertex.position.x, ComparisonTolerance);
				EXPECT_NEAR(vertex.data.y, vertex.position.y, ComparisonTolerance);
			}
		}
	}
}

TEST(Polygon2D, BuilderBakesImmutableVerticesAndComputesCounterClockwiseWinding)
{
	Polygon::Builder builder;
	builder.addVertex({{0.0f, 0.0f}, {0.0f, 0.0f}})
		.addVertex({{2.0f, 0.0f}, {2.0f, 0.0f}})
		.addVertex({{2.0f, 1.0f}, {2.0f, 1.0f}})
		.addVertex({{0.0f, 1.0f}, {0.0f, 1.0f}});

	const Polygon polygon = std::move(builder).bake();

	ASSERT_EQ(polygon.vertices().size(), 4u);
	EXPECT_EQ(polygon.vertices()[2].data, spk::Vector2(2.0f, 1.0f));
	EXPECT_FLOAT_EQ(polygon.signedArea(), 2.0f);
	EXPECT_FLOAT_EQ(polygon.area(), 2.0f);
	EXPECT_FALSE(polygon.isClockwise());
}

TEST(Polygon2D, BuilderComputesClockwiseWinding)
{
	const Polygon polygon = makeUnitSquare(true);

	EXPECT_FLOAT_EQ(polygon.signedArea(), -1.0f);
	EXPECT_FLOAT_EQ(polygon.area(), 1.0f);
	EXPECT_TRUE(polygon.isClockwise());
}

TEST(Polygon2D, BuilderRejectsFewerThanThreeVertices)
{
	Polygon::Builder builder;
	builder.addVertex({{0.0f, 0.0f}, {}})
		.addVertex({{1.0f, 0.0f}, {}});

	EXPECT_THROW((void)std::move(builder).bake(), std::logic_error);
}

TEST(Polygon2D, BuilderRejectsDegenerateContours)
{
	Polygon::Builder builder;
	builder.addVertex({{0.0f, 0.0f}, {}})
		.addVertex({{1.0f, 0.0f}, {}})
		.addVertex({{2.0f, 0.0f}, {}});

	EXPECT_THROW((void)std::move(builder).bake(), std::logic_error);
}

TEST(Polygon2D, ReportsAreaAndConvexityForEitherWinding)
{
	const Polygon counterClockwise = makeUnitSquare(false);
	const Polygon clockwise = makeUnitSquare(true);
	const Polygon concave = makePolygon({
		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{0.5f, 0.5f},
		{1.0f, 1.0f},
		{0.0f, 1.0f}});

	EXPECT_TRUE(counterClockwise.isConvex());
	EXPECT_TRUE(clockwise.isConvex());
	EXPECT_FALSE(concave.isConvex());
}

TEST(Polygon2D, IdenticalConvexPolygonFullyOccludesForEveryWindingCombination)
{
	for (const bool subjectClockwise : {false, true})
	{
		for (const bool clipClockwise : {false, true})
		{
			const Polygon subject = makeUnitSquare(subjectClockwise);
			const Polygon clip = makeUnitSquare(clipClockwise);

			const std::vector<Polygon> difference =
				subject.subtractConvex(clip, interpolatePayload);

			EXPECT_TRUE(difference.empty());
		}
	}
}

TEST(Polygon2D, ContainingPolygonFullyOccludesTheSubject)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon containingClip = makePolygon({
		{-1.0f, -1.0f},
		{2.0f, -1.0f},
		{2.0f, 2.0f},
		{-1.0f, 2.0f}});

	EXPECT_TRUE(subject.subtractConvex(containingClip, interpolatePayload).empty());
}

TEST(Polygon2D, DisjointPolygonLeavesTheSubjectUnchanged)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon disjointClip = makePolygon({
		{2.0f, 0.0f},
		{3.0f, 0.0f},
		{3.0f, 1.0f},
		{2.0f, 1.0f}});

	const std::vector<Polygon> difference =
		subject.subtractConvex(disjointClip, interpolatePayload);

	ASSERT_EQ(difference.size(), 1u);
	EXPECT_NEAR(difference.front().area(), 1.0f, ComparisonTolerance);
	expectPayloadMatchesPosition(difference);
}

TEST(Polygon2D, EdgeContactWithoutAreaOverlapLeavesTheSubjectUnchanged)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon touchingClip = makePolygon({
		{1.0f, 0.0f},
		{2.0f, 0.0f},
		{2.0f, 1.0f},
		{1.0f, 1.0f}});

	const std::vector<Polygon> difference =
		subject.subtractConvex(touchingClip, interpolatePayload);

	ASSERT_EQ(difference.size(), 1u);
	EXPECT_NEAR(totalArea(difference), 1.0f, ComparisonTolerance);
}

TEST(Polygon2D, PartialBoundaryOverlapRemovesOnlyTheIntersection)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon rightHalf = makePolygon({
		{0.5f, 0.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.5f, 1.0f}});

	const std::vector<Polygon> difference =
		subject.subtractConvex(rightHalf, interpolatePayload);

	ASSERT_EQ(difference.size(), 1u);
	EXPECT_NEAR(totalArea(difference), 0.5f, ComparisonTolerance);
	expectPayloadMatchesPosition(difference);
}

TEST(Polygon2D, PartialBoundaryOverlapAcceptsOppositeClipWinding)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon clockwiseRightHalf = makePolygon({
		{0.5f, 0.0f},
		{0.5f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f}});

	const std::vector<Polygon> difference =
		subject.subtractConvex(clockwiseRightHalf, interpolatePayload);

	ASSERT_EQ(difference.size(), 1u);
	EXPECT_NEAR(totalArea(difference), 0.5f, ComparisonTolerance);
	expectPayloadMatchesPosition(difference);
}

TEST(Polygon2D, InteriorOccluderProducesTheExpectedVisibleUnionArea)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon centralClip = makePolygon({
		{0.25f, 0.25f},
		{0.75f, 0.25f},
		{0.75f, 0.75f},
		{0.25f, 0.75f}});

	const std::vector<Polygon> difference =
		subject.subtractConvex(centralClip, interpolatePayload);

	ASSERT_EQ(difference.size(), 4u);
	EXPECT_NEAR(totalArea(difference), 0.75f, ComparisonTolerance);
	expectPayloadMatchesPosition(difference);
}

TEST(Polygon2D, GeneratedIntersectionVerticesInterpolateTheirPayload)
{
	const Polygon subject = makeUnitSquare(false);
	const Polygon overlappingClip = makePolygon({
		{0.5f, -0.5f},
		{1.5f, -0.5f},
		{1.5f, 0.5f},
		{0.5f, 0.5f}});

	const std::vector<Polygon> difference =
		subject.subtractConvex(overlappingClip, interpolatePayload);

	EXPECT_NEAR(totalArea(difference), 0.75f, ComparisonTolerance);
	expectPayloadMatchesPosition(difference);
}

TEST(Polygon2D, ConvexSubtractionRejectsNonConvexOperands)
{
	const Polygon convex = makeUnitSquare(false);
	const Polygon concave = makePolygon({
		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{0.5f, 0.5f},
		{1.0f, 1.0f},
		{0.0f, 1.0f}});

	EXPECT_THROW((void)concave.subtractConvex(convex, interpolatePayload), std::invalid_argument);
	EXPECT_THROW((void)convex.subtractConvex(concave, interpolatePayload), std::invalid_argument);
}
