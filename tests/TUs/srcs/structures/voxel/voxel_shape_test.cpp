#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_cell.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"

namespace
{
	class MissingSlotShape : public spk::VoxelShape
	{
	public:
		MissingSlotShape() :
			spk::VoxelShape({})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			mutableRenderFaces().outer(spk::VoxelAxisPlane::PositiveY) = createFace(createRectangle(
				"unknown", {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}));
		}
	};

	class InnerOnlyShape : public spk::VoxelShape
	{
	public:
		InnerOnlyShape() :
			spk::VoxelShape({{"quad", {0, 0}}})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			mutableRenderFaces().innerFaces.push_back(createFace(createRectangle(
				"quad", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0})));
		}
	};
}

TEST(VoxelShape, CubeShapeFillsTheOuterShellOnly)
{
	spk::CubeVoxelShape shape(spk::AtlasCell{0, 0});
	shape.initialize();

	EXPECT_TRUE(shape.initialized());
	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 6u);
	EXPECT_TRUE(shape.renderFaces().innerFaces.empty());
	for (const auto &face : shape.renderFaces().outerShell)
	{
		ASSERT_TRUE(face.has_value());
		ASSERT_EQ(face->polygons.size(), 1u);
		EXPECT_EQ(face->polygons.front().size(), 4u);
	}
}

TEST(VoxelShape, InitializeIsIdempotent)
{
	InnerOnlyShape shape;
	shape.initialize();
	shape.initialize();

	EXPECT_EQ(shape.renderFaces().innerFaces.size(), 1u);
	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 0u);
}

TEST(VoxelShape, AtlasUVProjectsLocalUVsIntoTheCell)
{
	spk::CubeVoxelShape defaultAtlas(spk::AtlasCell{2, 1});
	EXPECT_EQ(defaultAtlas.atlasSize(), spk::Vector2Int(8, 8));
	const spk::Vector2 uv = defaultAtlas.atlasUV({2, 1}, {0.5f, 0.5f});
	EXPECT_FLOAT_EQ(uv.x, 2.5f / 8.0f);
	EXPECT_FLOAT_EQ(uv.y, 1.5f / 8.0f);

	spk::CubeVoxelShape customAtlas(spk::AtlasCell{1, 1}, spk::Vector2Int{4, 2});
	const spk::Vector2 customUV = customAtlas.atlasUV({1, 1}, {0.5f, 0.5f});
	EXPECT_FLOAT_EQ(customUV.x, 1.5f / 4.0f);
	EXPECT_FLOAT_EQ(customUV.y, 1.5f / 2.0f);
}

TEST(VoxelShape, UniformCubeUsesItsCellOnEveryFace)
{
	spk::CubeVoxelShape shape(spk::AtlasCell{3, 2});
	shape.initialize();

	for (const auto &face : shape.renderFaces().outerShell)
	{
		for (const spk::VoxelShapeVertex &vertex : face->polygons.front())
		{
			EXPECT_GE(vertex.uv.x, 3.0f / 8.0f);
			EXPECT_LE(vertex.uv.x, 4.0f / 8.0f);
			EXPECT_GE(vertex.uv.y, 2.0f / 8.0f);
			EXPECT_LE(vertex.uv.y, 3.0f / 8.0f);
		}
	}
}

TEST(VoxelShape, MissingTextureSlotThrows)
{
	MissingSlotShape shape;

	EXPECT_THROW(shape.initialize(), std::logic_error);
}

TEST(VoxelRegistry, AttributesSequentialIdsAndInitializesShapes)
{
	spk::VoxelRegistry registry;
	const std::int32_t first = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	const std::int32_t second = registry.registerShape(std::make_unique<InnerOnlyShape>());

	EXPECT_EQ(first, 0);
	EXPECT_EQ(second, 1);
	EXPECT_EQ(registry.size(), 2u);
	EXPECT_TRUE(registry.shape(first).initialized());
	EXPECT_TRUE(registry.shape(second).initialized());
	EXPECT_EQ(registry.shape(second).renderFaces().outerFaceCount(), 0u);
}

TEST(VoxelRegistry, RejectsInvalidLookupsAndNullShapes)
{
	spk::VoxelRegistry registry;

	EXPECT_THROW((void)registry.registerShape(nullptr), std::invalid_argument);
	EXPECT_EQ(registry.tryShape(0), nullptr);
	EXPECT_EQ(registry.tryShape(-1), nullptr);
	EXPECT_THROW((void)registry.shape(0), std::out_of_range);
	EXPECT_THROW((void)registry.shape(spk::VoxelCell::EmptyId), std::out_of_range);
}
