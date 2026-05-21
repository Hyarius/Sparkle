#include <gtest/gtest.h>

#include <array>

#include "rendering/spk_mesh_2d.hpp"

TEST(GenericMeshTest, AddTriangleStoresVerticesIndexesAndShape)
{
	spk::Mesh2D mesh;

	mesh.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});

	EXPECT_EQ(mesh.buffer().vertices.size(), 3);
	EXPECT_EQ(mesh.buffer().indexes, (std::vector<std::uint32_t>{0, 1, 2}));
	ASSERT_EQ(mesh.shapes().size(), 1);
	EXPECT_EQ(mesh.shapes()[0].size(), 3);
}

TEST(GenericMeshTest, AddQuadStoresTwoTriangles)
{
	spk::Mesh2D mesh;

	mesh.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 1.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});

	EXPECT_EQ(mesh.buffer().vertices.size(), 4);
	EXPECT_EQ(mesh.buffer().indexes, (std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3}));
	ASSERT_EQ(mesh.shapes().size(), 1);
	EXPECT_EQ(mesh.shapes()[0].size(), 4);
}

TEST(GenericMeshTest, AddPolygonTriangulatesAsFan)
{
	spk::Mesh2D mesh;
	const std::array<spk::Vertex2D, 5> vertices{
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 1.0f}},
		spk::Vertex2D{.position = {0.5f, 1.5f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}}
	};

	mesh.addShape(std::span<const spk::Vertex2D>(vertices.data(), vertices.size()));

	EXPECT_EQ(mesh.buffer().vertices.size(), 5);
	EXPECT_EQ(mesh.buffer().indexes, (std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3, 0, 3, 4}));
}

TEST(GenericMeshTest, CopyAndMovePreserveShapeAccess)
{
	spk::Mesh2D mesh;
	mesh.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});

	spk::Mesh2D copy(mesh);
	EXPECT_EQ(copy.buffer().indexes, mesh.buffer().indexes);
	ASSERT_EQ(copy.shapes().size(), 1);
	EXPECT_EQ(copy.shapes()[0].size(), 3);

	spk::Mesh2D moved(std::move(copy));
	EXPECT_EQ(moved.buffer().indexes, mesh.buffer().indexes);
	ASSERT_EQ(moved.shapes().size(), 1);
	EXPECT_EQ(moved.shapes()[0].size(), 3);
}

TEST(GenericMeshTest, ClearRemovesBufferAndShapes)
{
	spk::Mesh2D mesh;
	mesh.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});

	mesh.clear();

	EXPECT_TRUE(mesh.buffer().vertices.empty());
	EXPECT_TRUE(mesh.buffer().indexes.empty());
	EXPECT_TRUE(mesh.shapes().empty());
}
