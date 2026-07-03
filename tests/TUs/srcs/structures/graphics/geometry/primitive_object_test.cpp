#include <gtest/gtest.h>

#include "structures/graphics/geometry/spk_primitive_object.hpp"

TEST(PrimitiveObject, CreateSquare_ShouldBuildTexturedQuadFromAnchorAndSize)
{
	const spk::TextureMesh2D mesh =
		spk::PrimitiveObject::CreateSquare({-0.5f, -0.25f}, {2.0f, 3.0f}, {0.25f, 0.5f}, {0.125f, 0.25f});

	const std::span<const spk::TextureVertex2D> vertices = mesh.vertices();
	const std::span<const spk::TextureMesh2D::Index> indexes = mesh.indexes();

	ASSERT_EQ(vertices.size(), 4u);
	ASSERT_EQ(indexes.size(), 6u);

	EXPECT_EQ(vertices[0].position, spk::Vector3(-0.5f, 2.75f, 0.0f));
	EXPECT_EQ(vertices[0].uv, spk::Vector2(0.25f, 0.5f));

	EXPECT_EQ(vertices[1].position, spk::Vector3(-0.5f, -0.25f, 0.0f));
	EXPECT_EQ(vertices[1].uv, spk::Vector2(0.25f, 0.75f));

	EXPECT_EQ(vertices[2].position, spk::Vector3(1.5f, -0.25f, 0.0f));
	EXPECT_EQ(vertices[2].uv, spk::Vector2(0.375f, 0.75f));

	EXPECT_EQ(vertices[3].position, spk::Vector3(1.5f, 2.75f, 0.0f));
	EXPECT_EQ(vertices[3].uv, spk::Vector2(0.375f, 0.5f));

	EXPECT_EQ(indexes[0], 0u);
	EXPECT_EQ(indexes[1], 1u);
	EXPECT_EQ(indexes[2], 2u);
	EXPECT_EQ(indexes[3], 0u);
	EXPECT_EQ(indexes[4], 2u);
	EXPECT_EQ(indexes[5], 3u);
}
