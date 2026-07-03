#include <gtest/gtest.h>

#include <algorithm>

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"

TEST(TextureVertex3D, EqualityAndHashIncludeNormal)
{
	const spk::TextureVertex3D first{{1.0f, 2.0f, 3.0f}, {0.0f, 1.0f, 0.0f}, {0.25f, 0.75f}};
	const spk::TextureVertex3D same = first;
	const spk::TextureVertex3D differentNormal{
		{1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f}, {0.25f, 0.75f}};
	EXPECT_EQ(first, same);
	EXPECT_NE(first, differentNormal);
	EXPECT_EQ(std::hash<spk::TextureVertex3D>{}(first), std::hash<spk::TextureVertex3D>{}(same));
}

TEST(TextureMesh3D, DeduplicatesEqualVerticesButSplitsHardEdgeNormals)
{
	const spk::TextureVertex3D a{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
	const spk::TextureVertex3D b{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}};
	const spk::TextureVertex3D c{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
	spk::TextureMesh3D::Builder builder;
	builder.addShape(a, b, c);
	builder.addShape(a, b, c);
	builder.addShape(
		spk::TextureVertex3D{a.position, {1.0f, 0.0f, 0.0f}, a.uv},
		spk::TextureVertex3D{b.position, {1.0f, 0.0f, 0.0f}, b.uv},
		spk::TextureVertex3D{c.position, {1.0f, 0.0f, 0.0f}, c.uv});
	const spk::TextureMesh3D mesh = builder.bake();
	EXPECT_EQ(mesh.vertices().size(), 6u);
	EXPECT_EQ(mesh.indexes().size(), 9u);
}

TEST(PrimitiveObject, CreateCubeBuildsCenteredHardEdgedTextureMesh3D)
{
	const spk::TextureMesh3D cube = spk::PrimitiveObject::CreateCube(2.0f);
	EXPECT_EQ(cube.vertices().size(), 24u);
	EXPECT_EQ(cube.indexes().size(), 36u);
	for (const spk::TextureVertex3D &vertex : cube.vertices())
	{
		EXPECT_NEAR(std::max({std::abs(vertex.position.x), std::abs(vertex.position.y), std::abs(vertex.position.z)}), 1.0f, 0.0001f);
		EXPECT_NEAR(vertex.normal.length(), 1.0f, 0.0001f);
	}
}

TEST(TextureMeshRenderer3D, StoresSharedMeshTexturePassAndTint)
{
	spk::Entity3D entity;
	auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube());
	spk::Texture texture;
	spk::TextureMeshRenderer3D &renderer = entity.addComponent<spk::TextureMeshRenderer3D>();
	renderer.setMesh(mesh);
	renderer.setTexture(&texture);
	renderer.setTranslucent(true);
	renderer.setTint({0.1f, 0.2f, 0.3f, 0.4f});

	EXPECT_EQ(renderer.mesh(), mesh);
	EXPECT_EQ(renderer.texture(), &texture);
	EXPECT_TRUE(renderer.translucent());
	EXPECT_FLOAT_EQ(renderer.tint().a, 0.4f);
}
