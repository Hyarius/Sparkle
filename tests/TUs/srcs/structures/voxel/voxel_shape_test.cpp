#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_cuboid_voxel_shape.hpp"
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
			mutableRenderFaces().outer(spk::VoxelAxisPlane::PositiveY).emplace(createRectangle("unknown", {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}));
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
			mutableRenderFaces().innerFaces.push_back(createRectangle(
				"quad", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0}));
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
		ASSERT_EQ(face->size(), 1u);
		EXPECT_EQ(face->polygons().front().size(), 4u);
	}
}

TEST(VoxelShape, CuboidShapeUsesAuthoredBounds)
{
	spk::CuboidVoxelShape shape(spk::AtlasCell{0, 0}, {0.25f, 0.0f, 0.35f}, {0.75f, 0.8f, 0.65f});
	shape.initialize();

	EXPECT_EQ(shape.minimum(), spk::Vector3(0.25f, 0.0f, 0.35f));
	EXPECT_EQ(shape.maximum(), spk::Vector3(0.75f, 0.8f, 0.65f));
	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 6u);
	EXPECT_TRUE(shape.outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane::NegativeY));
	EXPECT_FALSE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::NegativeY));
	EXPECT_FALSE(shape.outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane::PositiveX));
}

TEST(VoxelShape, CuboidShapeRejectsInvalidBounds)
{
	EXPECT_THROW(
		(void)spk::CuboidVoxelShape(spk::AtlasCell{0, 0}, {0.5f, 0.0f, 0.0f}, {0.5f, 1.0f, 1.0f}),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::CuboidVoxelShape(spk::AtlasCell{0, 0}, {-0.1f, 0.0f, 0.0f}, {0.5f, 1.0f, 1.0f}),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::CuboidVoxelShape(spk::AtlasCell{0, 0}, {0.0f, 0.0f, 0.0f}, {1.1f, 1.0f, 1.0f}),
		std::invalid_argument);
}

TEST(VoxelShapeFace, RequiresAllPolygonsToShareTheirNormal)
{
	spk::VoxelShapePolygon::Builder firstBuilder;
	firstBuilder.addVertex({{0, 0, 0}, {}}).addVertex({{1, 0, 0}, {}}).addVertex({{0, 1, 0}, {}});
	spk::VoxelShapeFace face(firstBuilder.bake());

	spk::VoxelShapePolygon::Builder matchingBuilder;
	matchingBuilder.addVertex({{1, 0, 0}, {}}).addVertex({{1, 1, 0}, {}}).addVertex({{0, 1, 0}, {}});
	EXPECT_NO_THROW(face.addPolygon(matchingBuilder.bake()));

	spk::VoxelShapePolygon::Builder oppositeBuilder;
	oppositeBuilder.addVertex({{0, 1, 0}, {}}).addVertex({{1, 0, 0}, {}}).addVertex({{0, 0, 0}, {}});
	EXPECT_THROW(face.addPolygon(oppositeBuilder.bake()), std::logic_error);
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
		for (const spk::VoxelShapeVertex &vertex : face->polygons().front())
		{
			EXPECT_GE(vertex.data.x, 3.0f / 8.0f);
			EXPECT_LE(vertex.data.x, 4.0f / 8.0f);
			EXPECT_GE(vertex.data.y, 2.0f / 8.0f);
			EXPECT_LE(vertex.data.y, 3.0f / 8.0f);
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
