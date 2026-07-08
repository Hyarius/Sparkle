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
