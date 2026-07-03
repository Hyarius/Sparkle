#include "geometry/mesh3d.hpp"
#include "geometry/primitive3d.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <functional>

TEST(MeshVertex3D, EqualityAndHashIncludeNormal)
{
	const pg::MeshVertex3D first{{1.0f, 2.0f, 3.0f}, {0.0f, 1.0f, 0.0f}, {0.25f, 0.75f}};
	const pg::MeshVertex3D same = first;
	const pg::MeshVertex3D differentNormal{{1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f}, {0.25f, 0.75f}};

	EXPECT_EQ(first, same);
	EXPECT_EQ(std::hash<pg::MeshVertex3D>{}(first), std::hash<pg::MeshVertex3D>{}(same));
	EXPECT_FALSE(first == differentNormal);
}

TEST(Mesh3D, DeduplicatesEqualVerticesButSplitsHardEdgeNormals)
{
	const pg::MeshVertex3D a{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
	const pg::MeshVertex3D b{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}};
	const pg::MeshVertex3D c{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};

	pg::Mesh3D::Builder builder;
	builder.addShape(a, b, c);
	builder.addShape(a, b, c);

	builder.addShape(
		pg::MeshVertex3D{a.position, {1.0f, 0.0f, 0.0f}, a.uv},
		pg::MeshVertex3D{b.position, {1.0f, 0.0f, 0.0f}, b.uv},
		pg::MeshVertex3D{c.position, {1.0f, 0.0f, 0.0f}, c.uv});
	pg::Mesh3D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 6);
	EXPECT_EQ(mesh.indexes().size(), 9);
}

TEST(Primitive3D, CubeHasExpectedTopologyAndFaceNormals)
{
	const pg::Mesh3D cube = pg::makeCube(2.0f);

	EXPECT_EQ(cube.vertices().size(), 24);
	EXPECT_EQ(cube.indexes().size(), 36);
	EXPECT_EQ(cube.nbShape(), 6);

	for (const pg::MeshVertex3D &vertex : cube.vertices())
	{
		EXPECT_NEAR(vertex.normal.length(), 1.0f, 1.0e-6f);

		const int axisCount =
			(static_cast<int>(std::abs(vertex.normal.x) > 0.999f) +
			 static_cast<int>(std::abs(vertex.normal.y) > 0.999f) +
			 static_cast<int>(std::abs(vertex.normal.z) > 0.999f));
		EXPECT_EQ(axisCount, 1);
	}
}
