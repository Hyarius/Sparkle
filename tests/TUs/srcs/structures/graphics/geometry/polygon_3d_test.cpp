#include <gtest/gtest.h>

#include "structures/graphics/geometry/spk_polygon_3d.hpp"
#include "structures/math/spk_vector2.hpp"

TEST(Polygon3D, BuilderComputesTheNormalAndBakesImmutableVertices)
{
	spk::Polygon3D<spk::Vector2>::Builder builder;
	builder.addVertex({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}})
		.addVertex({{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}})
		.addVertex({{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});
	const spk::Polygon3D<spk::Vector2> polygon = builder.bake();

	EXPECT_EQ(polygon.normal(), spk::Vector3(0.0f, 0.0f, 1.0f));
	ASSERT_EQ(polygon.vertices().size(), 3u);
	EXPECT_EQ(polygon.vertices()[1].data, spk::Vector2(1.0f, 0.0f));
}

TEST(Polygon3D, BuilderRejectsCollinearFirstVertices)
{
	spk::Polygon3D<int>::Builder builder;
	builder.addVertex({{0.0f, 0.0f, 0.0f}, 1}).addVertex({{1.0f, 0.0f, 0.0f}, 2});

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
	builder.addVertex({{0.0f, 0.0f, 0.0f}, 1}).addVertex({{1.0f, 0.0f, 0.0f}, 2});

	EXPECT_THROW((void)builder.bake(), std::logic_error);
}

TEST(Polygon3D, ReportsAreaAndConvexityInItsOwnPlane)
{
	spk::Polygon3D<int>::Builder squareBuilder;
	squareBuilder.addVertex({{0.0f, 0.0f, 0.0f}, 0})
		.addVertex({{2.0f, 0.0f, 0.0f}, 0})
		.addVertex({{2.0f, 1.0f, 0.0f}, 0})
		.addVertex({{0.0f, 1.0f, 0.0f}, 0});
	const spk::Polygon3D<int> square = squareBuilder.bake();
	EXPECT_FLOAT_EQ(square.area(), 2.0f);
	EXPECT_TRUE(square.isConvex(0.0001f));

	spk::Polygon3D<int>::Builder concaveBuilder;
	concaveBuilder.addVertex({{0.0f, 0.0f, 0.0f}, 0})
		.addVertex({{1.0f, 0.0f, 0.0f}, 0})
		.addVertex({{0.5f, 0.5f, 0.0f}, 0})
		.addVertex({{1.0f, 1.0f, 0.0f}, 0})
		.addVertex({{0.0f, 1.0f, 0.0f}, 0});
	EXPECT_FALSE(concaveBuilder.bake().isConvex(0.0001f));
}

TEST(Polygon3D, ConvexSubtractionInterpolatesPayloadAndAcceptsOppositeWinding)
{
	spk::Polygon3D<spk::Vector2>::Builder subjectBuilder;
	subjectBuilder.addVertex({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}})
		.addVertex({{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}})
		.addVertex({{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}})
		.addVertex({{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});

	// The clip is on a parallel plane and its clockwise winding deliberately
	// opposes the subject normal.
	spk::Polygon3D<spk::Vector2>::Builder clipBuilder;
	clipBuilder.addVertex({{0.5f, 0.0f, 1.0f}, {0.5f, 0.0f}})
		.addVertex({{0.5f, 1.0f, 1.0f}, {0.5f, 1.0f}})
		.addVertex({{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}})
		.addVertex({{1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}});

	const auto difference = subjectBuilder.bake().subtractConvex(
		clipBuilder.bake(),
		[](const spk::Vector2 &p_from, const spk::Vector2 &p_to, float p_interpolation) {
			return p_from + (p_to - p_from) * p_interpolation;
		},
		0.0001f);

	ASSERT_EQ(difference.size(), 1u);
	EXPECT_NEAR(difference.front().area(), 0.5f, 0.0001f);
	for (const auto &vertex : difference.front())
	{
		EXPECT_NEAR(vertex.data.x, vertex.position.x, 0.0001f);
		EXPECT_NEAR(vertex.data.y, vertex.position.y, 0.0001f);
	}
}
