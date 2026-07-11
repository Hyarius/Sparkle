#include <gtest/gtest.h>

#include "structures/graphics/geometry/spk_polygon_3d.hpp"
#include "structures/math/spk_vector2.hpp"

#include <array>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
	using Polygon2D = spk::Polygon2D<spk::Vector2>;
	using Polygon3D = spk::Polygon3D<spk::Vector2>;
	using Plane = Polygon3D::AxisAlignedPlane;

	constexpr float ComparisonTolerance = 0.0001f;

	[[nodiscard]] spk::Vector2 interpolatePayload(
		const spk::Vector2 &p_from,
		const spk::Vector2 &p_to,
		float p_interpolation) noexcept
	{
		return p_from + (p_to - p_from) * p_interpolation;
	}

	[[nodiscard]] Polygon3D makePolygon(
		std::initializer_list<spk::Vector3> p_positions)
	{
		Polygon3D::Builder builder;
		builder.reserve(p_positions.size());
		for (const spk::Vector3 &position : p_positions)
		{
			builder.addVertex({
				.position = position,
				.data = {position.x, position.y}});
		}
		return std::move(builder).bake();
	}

	[[nodiscard]] float totalArea(const std::vector<Polygon2D> &p_polygons)
	{
		float result = 0.0f;
		for (const Polygon2D &polygon : p_polygons)
		{
			result += polygon.area();
		}
		return result;
	}

	[[nodiscard]] float totalArea(const std::vector<Polygon3D> &p_polygons)
	{
		float result = 0.0f;
		for (const Polygon3D &polygon : p_polygons)
		{
			result += polygon.area();
		}
		return result;
	}

	struct OppositeFacePair
	{
		Plane plane;
		Polygon3D positive;
		Polygon3D negative;
	};

	[[nodiscard]] std::array<OppositeFacePair, 3> makeOppositeFacePairs()
	{
		return {{
			{
				.plane = Plane::YZ,
				.positive = makePolygon({
					{1.0f, 0.0f, 1.0f},
					{1.0f, 0.0f, 0.0f},
					{1.0f, 1.0f, 0.0f},
					{1.0f, 1.0f, 1.0f}}),
				.negative = makePolygon({
					{0.0f, 0.0f, 0.0f},
					{0.0f, 0.0f, 1.0f},
					{0.0f, 1.0f, 1.0f},
					{0.0f, 1.0f, 0.0f}})},
			{
				.plane = Plane::XZ,
				.positive = makePolygon({
					{0.0f, 1.0f, 1.0f},
					{1.0f, 1.0f, 1.0f},
					{1.0f, 1.0f, 0.0f},
					{0.0f, 1.0f, 0.0f}}),
				.negative = makePolygon({
					{0.0f, 0.0f, 0.0f},
					{1.0f, 0.0f, 0.0f},
					{1.0f, 0.0f, 1.0f},
					{0.0f, 0.0f, 1.0f}})},
			{
				.plane = Plane::XY,
				.positive = makePolygon({
					{0.0f, 0.0f, 1.0f},
					{1.0f, 0.0f, 1.0f},
					{1.0f, 1.0f, 1.0f},
					{0.0f, 1.0f, 1.0f}}),
				.negative = makePolygon({
					{1.0f, 0.0f, 0.0f},
					{0.0f, 0.0f, 0.0f},
					{0.0f, 1.0f, 0.0f},
					{1.0f, 1.0f, 0.0f}})}
		}};
	}
}

TEST(Polygon3D, BuilderComputesTheNormalAndBakesImmutableVertices)
{
	Polygon3D::Builder builder;
	builder.addVertex({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}})
		.addVertex({{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}})
		.addVertex({{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});
	const Polygon3D polygon = std::move(builder).bake();

	EXPECT_EQ(polygon.normal(), spk::Vector3(0.0f, 0.0f, 1.0f));
	ASSERT_EQ(polygon.vertices().size(), 3u);
	EXPECT_EQ(polygon.vertices()[1].data, spk::Vector2(1.0f, 0.0f));
}

TEST(Polygon3D, BuilderRejectsCollinearFirstVertices)
{
	spk::Polygon3D<int>::Builder builder;
	builder.addVertex({{0.0f, 0.0f, 0.0f}, 1})
		.addVertex({{1.0f, 0.0f, 0.0f}, 2});

	EXPECT_THROW(builder.addVertex({{2.0f, 0.0f, 0.0f}, 3}), std::logic_error);
}

TEST(Polygon3D, BuilderRejectsVerticesOutsideThePolygonPlane)
{
	spk::Polygon3D<int>::Builder builder;
	builder.addVertex({{0.0f, 0.0f, 0.0f}, 1})
		.addVertex({{1.0f, 0.0f, 0.0f}, 2})
		.addVertex({{0.0f, 1.0f, 0.0f}, 3});

	EXPECT_THROW(builder.addVertex({{0.0f, 1.0f, 1.0f}, 4}), std::logic_error);
}

TEST(Polygon3D, BuilderRejectsFewerThanThreeVertices)
{
	spk::Polygon3D<int>::Builder builder;
	builder.addVertex({{0.0f, 0.0f, 0.0f}, 1})
		.addVertex({{1.0f, 0.0f, 0.0f}, 2});

	EXPECT_THROW((void)std::move(builder).bake(), std::logic_error);
}

TEST(Polygon3D, ReportsAreaAndConvexityInItsOwnPlane)
{
	spk::Polygon3D<int>::Builder squareBuilder;
	squareBuilder.addVertex({{0.0f, 0.0f, 0.0f}, 0})
		.addVertex({{2.0f, 0.0f, 0.0f}, 0})
		.addVertex({{2.0f, 1.0f, 0.0f}, 0})
		.addVertex({{0.0f, 1.0f, 0.0f}, 0});
	const spk::Polygon3D<int> square = std::move(squareBuilder).bake();
	EXPECT_FLOAT_EQ(square.area(), 2.0f);
	EXPECT_TRUE(square.isConvex(ComparisonTolerance));

	spk::Polygon3D<int>::Builder concaveBuilder;
	concaveBuilder.addVertex({{0.0f, 0.0f, 0.0f}, 0})
		.addVertex({{1.0f, 0.0f, 0.0f}, 0})
		.addVertex({{0.5f, 0.5f, 0.0f}, 0})
		.addVertex({{1.0f, 1.0f, 0.0f}, 0})
		.addVertex({{0.0f, 1.0f, 0.0f}, 0});
	EXPECT_FALSE(std::move(concaveBuilder).bake().isConvex(ComparisonTolerance));
}

TEST(Polygon3D, ConvexSubtractionInterpolatesPayloadAndAcceptsOppositeWinding)
{
	Polygon3D::Builder subjectBuilder;
	subjectBuilder.addVertex({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}})
		.addVertex({{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}})
		.addVertex({{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}})
		.addVertex({{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});

	Polygon3D::Builder clipBuilder;
	clipBuilder.addVertex({{0.5f, 0.0f, 1.0f}, {0.5f, 0.0f}})
		.addVertex({{0.5f, 1.0f, 1.0f}, {0.5f, 1.0f}})
		.addVertex({{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}})
		.addVertex({{1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}});

	const std::vector<Polygon3D> difference =
		std::move(subjectBuilder).bake().subtractConvex(
			std::move(clipBuilder).bake(),
			interpolatePayload,
			ComparisonTolerance);

	ASSERT_EQ(difference.size(), 1u);
	EXPECT_NEAR(difference.front().area(), 0.5f, ComparisonTolerance);
	for (const Polygon3D::Vertex &vertex : difference.front())
	{
		EXPECT_NEAR(vertex.data.x, vertex.position.x, ComparisonTolerance);
		EXPECT_NEAR(vertex.data.y, vertex.position.y, ComparisonTolerance);
	}
}

TEST(Polygon3D, ProjectionPlaneRecognizesBothDirectionsOfEveryAxis)
{
	for (const OppositeFacePair &pair : makeOppositeFacePairs())
	{
		const std::optional<Plane> positivePlane = pair.positive.projectionPlane();
		const std::optional<Plane> negativePlane = pair.negative.projectionPlane();

		ASSERT_TRUE(positivePlane.has_value());
		ASSERT_TRUE(negativePlane.has_value());
		EXPECT_EQ(*positivePlane, pair.plane);
		EXPECT_EQ(*negativePlane, pair.plane);
		EXPECT_TRUE(pair.positive.isProjectable());
		EXPECT_TRUE(pair.negative.isProjectable());
	}
}

TEST(Polygon3D, TryProjectTo2DPreservesTheRemovedCoordinateAndPayload)
{
	const Polygon3D polygon = makePolygon({
		{1.0f, 2.0f, 4.0f},
		{3.0f, 2.0f, 4.0f},
		{3.0f, 5.0f, 4.0f},
		{1.0f, 5.0f, 4.0f}});

	const std::optional<Polygon3D::Projection2D> projection = polygon.tryProjectTo2D();

	ASSERT_TRUE(projection.has_value());
	EXPECT_EQ(projection->plane, Plane::XY);
	EXPECT_FLOAT_EQ(projection->coordinate, 4.0f);
	ASSERT_EQ(projection->polygon.size(), polygon.size());
	for (std::size_t index = 0; index < polygon.size(); ++index)
	{
		EXPECT_EQ(projection->polygon[index].position,
			spk::Vector2(polygon[index].position.x, polygon[index].position.y));
		EXPECT_EQ(projection->polygon[index].data, polygon[index].data);
	}
}

TEST(Polygon3D, TryProjectTo2DUsesTheExpectedCoordinatesForXZAndYZ)
{
	const Polygon3D xzPolygon = makePolygon({
		{1.0f, 3.0f, 2.0f},
		{1.0f, 3.0f, 5.0f},
		{4.0f, 3.0f, 5.0f},
		{4.0f, 3.0f, 2.0f}});
	const Polygon3D yzPolygon = makePolygon({
		{6.0f, 1.0f, 2.0f},
		{6.0f, 4.0f, 2.0f},
		{6.0f, 4.0f, 5.0f},
		{6.0f, 1.0f, 5.0f}});

	const auto xzProjection = xzPolygon.tryProjectTo2D();
	const auto yzProjection = yzPolygon.tryProjectTo2D();

	ASSERT_TRUE(xzProjection.has_value());
	ASSERT_TRUE(yzProjection.has_value());
	EXPECT_EQ(xzProjection->plane, Plane::XZ);
	EXPECT_EQ(yzProjection->plane, Plane::YZ);
	EXPECT_FLOAT_EQ(xzProjection->coordinate, 3.0f);
	EXPECT_FLOAT_EQ(yzProjection->coordinate, 6.0f);
	EXPECT_EQ(xzProjection->polygon[2].position, spk::Vector2(4.0f, 5.0f));
	EXPECT_EQ(yzProjection->polygon[2].position, spk::Vector2(4.0f, 5.0f));
}

TEST(Polygon3D, ObliquePolygonIsNotProjectable)
{
	const Polygon3D polygon = makePolygon({
		{0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{0.0f, 1.0f, 0.0f}});

	EXPECT_FALSE(polygon.projectionPlane().has_value());
	EXPECT_FALSE(polygon.isProjectable());
	EXPECT_FALSE(polygon.tryProjectTo2D().has_value());
}

TEST(Polygon3D, OppositeVoxelFacesProjectToEquivalentFootprints)
{
	for (const OppositeFacePair &pair : makeOppositeFacePairs())
	{
		const auto positiveProjection = pair.positive.tryProjectTo2D();
		const auto negativeProjection = pair.negative.tryProjectTo2D();

		ASSERT_TRUE(positiveProjection.has_value());
		ASSERT_TRUE(negativeProjection.has_value());
		EXPECT_EQ(positiveProjection->plane, negativeProjection->plane);
		EXPECT_NE(positiveProjection->coordinate, negativeProjection->coordinate);

		const std::vector<Polygon2D> positiveMinusNegative =
			positiveProjection->polygon.subtractConvex(
				negativeProjection->polygon,
				interpolatePayload);
		const std::vector<Polygon2D> negativeMinusPositive =
			negativeProjection->polygon.subtractConvex(
				positiveProjection->polygon,
				interpolatePayload);

		EXPECT_TRUE(positiveMinusNegative.empty());
		EXPECT_TRUE(negativeMinusPositive.empty());
	}
}

TEST(Polygon3D, ProjectedPartialSubtractionMatchesLegacy3DSubtractionOnEveryAxis)
{
	const std::array<std::pair<Polygon3D, Polygon3D>, 3> cases = {{
		{
			makePolygon({
				{0.0f, 0.0f, 0.0f},
				{1.0f, 0.0f, 0.0f},
				{1.0f, 1.0f, 0.0f},
				{0.0f, 1.0f, 0.0f}}),
			makePolygon({
				{0.5f, 0.0f, 1.0f},
				{0.5f, 1.0f, 1.0f},
				{1.0f, 1.0f, 1.0f},
				{1.0f, 0.0f, 1.0f}})},
		{
			makePolygon({
				{0.0f, 0.0f, 0.0f},
				{0.0f, 0.0f, 1.0f},
				{1.0f, 0.0f, 1.0f},
				{1.0f, 0.0f, 0.0f}}),
			makePolygon({
				{0.5f, 1.0f, 0.0f},
				{1.0f, 1.0f, 0.0f},
				{1.0f, 1.0f, 1.0f},
				{0.5f, 1.0f, 1.0f}})},
		{
			makePolygon({
				{0.0f, 0.0f, 0.0f},
				{0.0f, 1.0f, 0.0f},
				{0.0f, 1.0f, 1.0f},
				{0.0f, 0.0f, 1.0f}}),
			makePolygon({
				{1.0f, 0.5f, 0.0f},
				{1.0f, 0.5f, 1.0f},
				{1.0f, 1.0f, 1.0f},
				{1.0f, 1.0f, 0.0f}})}
	}};

	for (const auto &[subject, clip] : cases)
	{
		const std::vector<Polygon3D> difference3D =
			subject.subtractConvex(clip, interpolatePayload, ComparisonTolerance);

		const auto subjectProjection = subject.tryProjectTo2D();
		const auto clipProjection = clip.tryProjectTo2D();
		ASSERT_TRUE(subjectProjection.has_value());
		ASSERT_TRUE(clipProjection.has_value());
		ASSERT_EQ(subjectProjection->plane, clipProjection->plane);

		const std::vector<Polygon2D> difference2D =
			subjectProjection->polygon.subtractConvex(
				clipProjection->polygon,
				interpolatePayload);

		EXPECT_NEAR(totalArea(difference3D), 0.5f, ComparisonTolerance);
		EXPECT_NEAR(totalArea(difference2D), 0.5f, ComparisonTolerance);
		EXPECT_NEAR(totalArea(difference2D), totalArea(difference3D), ComparisonTolerance);
	}
}

TEST(Polygon3D, ProjectedFullOverlapMatchesLegacy3DSubtractionOnEveryAxis)
{
	for (const OppositeFacePair &pair : makeOppositeFacePairs())
	{
		const std::vector<Polygon3D> difference3D =
			pair.positive.subtractConvex(
				pair.negative,
				interpolatePayload,
				ComparisonTolerance);

		const auto positiveProjection = pair.positive.tryProjectTo2D();
		const auto negativeProjection = pair.negative.tryProjectTo2D();
		ASSERT_TRUE(positiveProjection.has_value());
		ASSERT_TRUE(negativeProjection.has_value());

		const std::vector<Polygon2D> difference2D =
			positiveProjection->polygon.subtractConvex(
				negativeProjection->polygon,
				interpolatePayload);

		EXPECT_TRUE(difference3D.empty());
		EXPECT_TRUE(difference2D.empty());
	}
}
