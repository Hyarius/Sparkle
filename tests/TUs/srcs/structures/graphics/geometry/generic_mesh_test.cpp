#include <gtest/gtest.h>

#include <array>
#include <concepts>
#include <span>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "structures/graphics/geometry/spk_color_mesh_2d.hpp"
#include "structures/graphics/geometry/spk_mesh_2d.hpp"

namespace
{
	struct InvalidMeshLayout
	{
	};

	using Attribute = spk::LayoutBufferObject::Attribute;
	using NonSequentialMeshLayout = spk::MeshLayout<
		Attribute{7, Attribute::Type::Vector2},
		Attribute{3, Attribute::Type::Vector2}>;
	using NonSequentialMesh = spk::GenericMesh<spk::Vertex2D, NonSequentialMeshLayout>;

	static_assert(spk::mesh_layout<NonSequentialMeshLayout>);
	static_assert(!spk::mesh_layout<InvalidMeshLayout>);
	static_assert(std::same_as<decltype(std::declval<const spk::Mesh2D::Builder &>().bake()), spk::Mesh2D>);
	static_assert(std::same_as<decltype(std::declval<const spk::ColorMesh2D::Builder &>().bake()), spk::ColorMesh2D>);
	static_assert(!std::copy_constructible<spk::Mesh2D::Builder>);

	template <typename T>
	[[nodiscard]] std::vector<T> toVector(std::span<const T> p_values)
	{
		return std::vector<T>(p_values.begin(), p_values.end());
	}
}

TEST(GenericMeshTest, LayoutPreservesExplicitAttributeLocations)
{
	NonSequentialMesh mesh;

	EXPECT_TRUE(mesh.layoutBuffer().hasAttribute(7));
	EXPECT_TRUE(mesh.layoutBuffer().hasAttribute(3));
	EXPECT_FALSE(mesh.layoutBuffer().hasAttribute(0));
}

TEST(GenericMeshTest, AddTriangleStoresVerticesIndexesAndShape)
{
	spk::Mesh2D::Builder builder;

	builder.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});
	spk::Mesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 3);
	EXPECT_EQ(toVector(mesh.indexes()), (std::vector<std::uint32_t>{0, 1, 2}));
	ASSERT_EQ(mesh.shapes().size(), 1);
	EXPECT_EQ(mesh.shapes()[0].size(), 3);
}

TEST(GenericMeshTest, AddQuadStoresTwoTriangles)
{
	spk::Mesh2D::Builder builder;

	builder.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 1.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});
	spk::Mesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 4);
	EXPECT_EQ(toVector(mesh.indexes()), (std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3}));
	ASSERT_EQ(mesh.shapes().size(), 1);
	EXPECT_EQ(mesh.shapes()[0].size(), 4);
}

TEST(GenericMeshTest, AddPolygonTriangulatesAsFan)
{
	spk::Mesh2D::Builder builder;
	const std::array<spk::Vertex2D, 5> vertices{
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 1.0f}},
		spk::Vertex2D{.position = {0.5f, 1.5f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}}};

	builder.addShape(std::span<const spk::Vertex2D>(vertices.data(), vertices.size()));
	spk::Mesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 5);
	EXPECT_EQ(toVector(mesh.indexes()), (std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3, 0, 3, 4}));
}

TEST(GenericMeshTest, CopyAndMovePreserveShapeAccess)
{
	spk::Mesh2D::Builder builder;
	builder.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});
	spk::Mesh2D mesh = builder.bake();

	spk::Mesh2D copy(mesh);
	EXPECT_EQ(toVector(copy.indexes()), toVector(mesh.indexes()));
	ASSERT_EQ(copy.shapes().size(), 1);
	EXPECT_EQ(copy.shapes()[0].size(), 3);

	spk::Mesh2D moved(std::move(copy));
	EXPECT_EQ(toVector(moved.indexes()), toVector(mesh.indexes()));
	ASSERT_EQ(moved.shapes().size(), 1);
	EXPECT_EQ(moved.shapes()[0].size(), 3);
}

TEST(GenericMeshTest, ClearRemovesBufferAndShapes)
{
	spk::Mesh2D::Builder builder;
	builder.addShape(
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}},
		spk::Vertex2D{.position = {0.0f, 1.0f}});
	builder.clear();
	spk::Mesh2D mesh = builder.bake();

	EXPECT_TRUE(mesh.vertices().empty());
	EXPECT_TRUE(mesh.indexes().empty());
	EXPECT_TRUE(mesh.shapes().empty());
}

TEST(GenericMeshTest, ThrowWithShapesWithFewerThanThreeVertices)
{
	spk::Mesh2D::Builder builder;
	const std::array<spk::Vertex2D, 2> vertices{
		spk::Vertex2D{.position = {0.0f, 0.0f}},
		spk::Vertex2D{.position = {1.0f, 0.0f}}};

	EXPECT_THROW(builder.addShape(std::span<const spk::Vertex2D>(vertices.data(), vertices.size())), std::runtime_error);
	EXPECT_THROW(builder.addShape(std::vector<spk::Vertex2D>{}), std::runtime_error);
	spk::Mesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.nbShape(), 0u);
	EXPECT_TRUE(mesh.vertices().empty());
}

TEST(GenericMeshTest, ReusesDuplicateVerticesAcrossShapes)
{
	spk::Mesh2D::Builder builder;
	const spk::Vertex2D a{.position = {0.0f, 0.0f}};
	const spk::Vertex2D b{.position = {1.0f, 0.0f}};
	const spk::Vertex2D c{.position = {0.0f, 1.0f}};

	builder.reserve(6, 6);
	builder.addShape(a, b, c);
	builder.addShape(a, c, b);
	spk::Mesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 3u);
	EXPECT_EQ(toVector(mesh.indexes()), (std::vector<std::uint32_t>{0, 1, 2, 0, 2, 1}));
	EXPECT_EQ(mesh.nbShape(), 2u);
}

TEST(GenericMeshTest, ReusesDuplicateVerticesInsideShape)
{
	spk::Mesh2D::Builder builder;
	const spk::Vertex2D a{.position = {0.0f, 0.0f}};
	const spk::Vertex2D b{.position = {1.0f, 0.0f}};
	const std::array<spk::Vertex2D, 3> vertices{a, b, a};

	builder.addShape(std::span<const spk::Vertex2D>(vertices.data(), vertices.size()));
	spk::Mesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 2u);
	EXPECT_EQ(toVector(mesh.indexes()), (std::vector<std::uint32_t>{0, 1, 0}));
	ASSERT_EQ(mesh.shapes().size(), 1u);
	EXPECT_EQ(mesh.shapes()[0], (std::vector<std::uint32_t>{0, 1, 0}));
}

TEST(GenericMeshTest, BuilderCanBakeMultipleIndependentSnapshots)
{
	spk::Mesh2D::Builder builder;
	const spk::Vertex2D a{.position = {0.0f, 0.0f}};
	const spk::Vertex2D b{.position = {1.0f, 0.0f}};
	const spk::Vertex2D c{.position = {0.0f, 1.0f}};

	builder.addShape(a, b, c);
	const spk::Mesh2D first = builder.bake();

	builder.addShape(a, c, b);
	const spk::Mesh2D second = builder.bake();

	EXPECT_EQ(first.nbShape(), 1u);
	EXPECT_EQ(first.indexes().size(), 3u);
	EXPECT_EQ(second.nbShape(), 2u);
	EXPECT_EQ(second.indexes().size(), 6u);
}

TEST(ColorMesh2DTest, ColorValuesVertexComparisonAndHashWork)
{
	const spk::Color color(0.1f, 0.2f, 0.3f, 0.4f);
	const spk::ColorVertex2D first{.position = {1.0f, 2.0f, 0.0f}, .color = color};
	const spk::ColorVertex2D same{.position = {1.0f, 2.0f, 0.0f}, .color = color};
	const spk::ColorVertex2D other{.position = {2.0f, 1.0f, 0.0f}, .color = spk::Color(0.4f, 0.3f, 0.2f, 0.1f)};

	EXPECT_EQ(color.values(), (std::array<float, 4>{0.1f, 0.2f, 0.3f, 0.4f}));
	EXPECT_EQ(first, same);
	EXPECT_FALSE(first == other);

	std::unordered_set<spk::ColorVertex2D> vertices;
	vertices.insert(first);
	vertices.insert(same);
	vertices.insert(other);

	EXPECT_EQ(vertices.size(), 2u);
}

TEST(ColorMesh2DTest, StoresColoredShape)
{
	spk::ColorMesh2D::Builder builder;

	builder.addShape(
		spk::ColorVertex2D{.position = {0.0f, 0.0f, 0.0f}, .color = spk::Color(1.0f, 0.0f, 0.0f)},
		spk::ColorVertex2D{.position = {1.0f, 0.0f, 0.0f}, .color = spk::Color(0.0f, 1.0f, 0.0f)},
		spk::ColorVertex2D{.position = {0.0f, 1.0f, 0.0f}, .color = spk::Color(0.0f, 0.0f, 1.0f)});
	spk::ColorMesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.vertices().size(), 3u);
	EXPECT_EQ(toVector(mesh.indexes()), (std::vector<std::uint32_t>{0, 1, 2}));
}
