#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_data_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

namespace
{
	const spk::VoxelShape::TextureSlots UniformCubeSlots = {
		{"posX", {0, 0}}, {"negX", {0, 0}}, {"top", {0, 0}}, {"bottom", {0, 0}}, {"posZ", {0, 0}}, {"negZ", {0, 0}}};
	const spk::VoxelShape::TextureSlots UniformSideSlots = {
		{"top", {0, 0}}, {"bottom", {0, 0}}, {"side", {0, 0}}};

	const std::vector<spk::Vector2> RectangleUVs = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
	const std::vector<spk::Vector2> VerticalRectangleUVs = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

	[[nodiscard]] std::vector<spk::Vector2> verticalUVs(float p_bottomV, float p_topV)
	{
		return {{0, p_bottomV}, {1, p_bottomV}, {1, p_topV}, {0, p_topV}};
	}

	// Mirrors spk::CubeVoxelShape::_constructRenderFaces.
	[[nodiscard]] spk::VoxelShapeDescription cubeDescription()
	{
		spk::VoxelShapeDescription description;
		description.polygons = {
			{"posX", {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}, VerticalRectangleUVs},
			{"negX", {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, VerticalRectangleUVs},
			{"top", {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}, RectangleUVs},
			{"bottom", {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, RectangleUVs},
			{"posZ", {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, VerticalRectangleUVs},
			{"negZ", {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}, VerticalRectangleUVs},
		};
		return description;
	}

	// Mirrors spk::SlabVoxelShape::_constructRenderFaces at height 0.5.
	[[nodiscard]] spk::VoxelShapeDescription slabDescription()
	{
		const float height = 0.5f;
		spk::VoxelShapeDescription description;
		description.polygons = {
			{"top", {{0, height, 1}, {1, height, 1}, {1, height, 0}, {0, height, 0}}, RectangleUVs},
			{"bottom", {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, RectangleUVs},
			{"side", {{1, 0, 1}, {1, 0, 0}, {1, height, 0}, {1, height, 1}}, verticalUVs(1.0f, 1.0f - height)},
			{"side", {{0, 0, 0}, {0, 0, 1}, {0, height, 1}, {0, height, 0}}, verticalUVs(1.0f, 1.0f - height)},
			{"side", {{0, 0, 1}, {1, 0, 1}, {1, height, 1}, {0, height, 1}}, verticalUVs(1.0f, 1.0f - height)},
			{"side", {{1, 0, 0}, {0, 0, 0}, {0, height, 0}, {1, height, 0}}, verticalUVs(1.0f, 1.0f - height)},
		};
		return description;
	}

	void expectFacesEqual(const spk::VoxelShapeFace &p_actual, const spk::VoxelShapeFace &p_expected)
	{
		ASSERT_EQ(p_actual.size(), p_expected.size());
		for (std::size_t polygonIndex = 0; polygonIndex < p_actual.size(); ++polygonIndex)
		{
			const spk::VoxelShapePolygon &actual = p_actual.polygons()[polygonIndex];
			const spk::VoxelShapePolygon &expected = p_expected.polygons()[polygonIndex];
			EXPECT_EQ(actual.normal(), expected.normal());
			ASSERT_EQ(actual.size(), expected.size());
			for (std::size_t vertexIndex = 0; vertexIndex < actual.size(); ++vertexIndex)
			{
				EXPECT_EQ(actual[vertexIndex].position, expected[vertexIndex].position);
				EXPECT_EQ(actual[vertexIndex].data, expected[vertexIndex].data);
			}
		}
	}
}

TEST(DataVoxelShape, CubeDescriptionMatchesCubeVoxelShape)
{
	spk::DataVoxelShape dataShape(UniformCubeSlots, cubeDescription());
	dataShape.initialize();

	spk::CubeVoxelShape referenceShape(UniformCubeSlots);
	referenceShape.initialize();

	EXPECT_TRUE(dataShape.renderFaces().innerFaces.empty());
	ASSERT_EQ(dataShape.renderFaces().outerFaceCount(), 6u);
	for (std::size_t planeIndex = 0; planeIndex < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++planeIndex)
	{
		const auto plane = static_cast<spk::VoxelAxisPlane>(planeIndex);
		ASSERT_TRUE(dataShape.renderFaces().outer(plane).has_value());
		expectFacesEqual(*dataShape.renderFaces().outer(plane), *referenceShape.renderFaces().outer(plane));
		EXPECT_TRUE(dataShape.outerFaceLiesOnCellBoundary(plane));
		EXPECT_TRUE(dataShape.outerFaceCoversCellBoundary(plane));
	}
}

TEST(DataVoxelShape, SlabTopIsClassifiedInner)
{
	spk::DataVoxelShape shape(UniformSideSlots, slabDescription());
	shape.initialize();

	// The bottom and the four boundary sides join the outer shell; the mid-height top is
	// inside the cell and must become an inner face (unlike the legacy SlabVoxelShape,
	// which pinned it to the PositiveY outer slot where it could never be culled).
	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 5u);
	EXPECT_FALSE(shape.renderFaces().outer(spk::VoxelAxisPlane::PositiveY).has_value());
	ASSERT_EQ(shape.renderFaces().innerFaces.size(), 1u);
	EXPECT_EQ(shape.renderFaces().innerFaces.front().polygons().front().normal(), spk::Vector3(0, 1, 0));

	EXPECT_TRUE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::NegativeY));
	EXPECT_TRUE(shape.outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane::PositiveX));
	EXPECT_FALSE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::PositiveX));
	EXPECT_FLOAT_EQ(shape.outerFaceBoundaryCoverage(spk::VoxelAxisPlane::PositiveX), 0.5f);
}

TEST(DataVoxelShape, InwardFacingBoundaryPolygonStaysInner)
{
	// Lies on the y = 1 plane but winds so its normal points down, into the cell: it can
	// neither seal the boundary nor be sealed by the neighbor above, so it stays inner.
	spk::VoxelShapeDescription description;
	description.polygons = {
		{"top", {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}, RectangleUVs},
	};
	spk::DataVoxelShape shape(UniformSideSlots, std::move(description));
	shape.initialize();

	EXPECT_FALSE(shape.hasOuterFaces());
	ASSERT_EQ(shape.renderFaces().innerFaces.size(), 1u);
	EXPECT_EQ(shape.renderFaces().innerFaces.front().polygons().front().normal(), spk::Vector3(0, -1, 0));
}

TEST(DataVoxelShape, BoundaryPolygonsOnOnePlaneMergeIntoOneOuterFace)
{
	spk::VoxelShapeDescription description;
	description.polygons = {
		{"top", {{0, 1, 1}, {0.5, 1, 1}, {0.5, 1, 0}, {0, 1, 0}}, RectangleUVs},
		{"top", {{0.5, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0.5, 1, 0}}, RectangleUVs},
	};
	spk::DataVoxelShape shape(UniformSideSlots, std::move(description));
	shape.initialize();

	const auto &face = shape.renderFaces().outer(spk::VoxelAxisPlane::PositiveY);
	ASSERT_TRUE(face.has_value());
	EXPECT_EQ(face->size(), 2u);
	EXPECT_TRUE(shape.outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane::PositiveY));
	EXPECT_TRUE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::PositiveY));
}

TEST(DataVoxelShape, NearBoundaryVerticesAreSnappedOntoThePlane)
{
	spk::VoxelShapeDescription description;
	description.polygons = {
		{"top", {{0, 0.99995f, 1}, {1, 0.99995f, 1}, {1, 1.00005f, 0}, {0, 1.00005f, 0}}, RectangleUVs},
	};
	spk::DataVoxelShape shape(UniformSideSlots, std::move(description));
	shape.initialize();

	ASSERT_TRUE(shape.renderFaces().outer(spk::VoxelAxisPlane::PositiveY).has_value());
	EXPECT_TRUE(shape.outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane::PositiveY));
	EXPECT_TRUE(shape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::PositiveY));
	for (const spk::VoxelShapeVertex &vertex : shape.renderFaces().outer(spk::VoxelAxisPlane::PositiveY)->polygons().front())
	{
		EXPECT_EQ(vertex.position.y, 1.0f);
	}
}

TEST(DataVoxelShape, DiagonalGeometryStaysInner)
{
	spk::VoxelShapeDescription description;
	description.polygons = {
		{"top", {{0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0}}, VerticalRectangleUVs},
	};
	spk::DataVoxelShape shape(UniformSideSlots, std::move(description));
	shape.initialize();

	EXPECT_FALSE(shape.hasOuterFaces());
	EXPECT_EQ(shape.renderFaces().innerFaces.size(), 1u);
}

TEST(DataVoxelShape, RejectsInvalidDescriptions)
{
	EXPECT_THROW(
		(void)spk::DataVoxelShape(UniformSideSlots, spk::VoxelShapeDescription{}),
		std::invalid_argument);

	spk::VoxelShapeDescription twoVertices;
	twoVertices.polygons = {{"top", {{0, 1, 1}, {1, 1, 1}}, {{0, 0}, {1, 0}}}};
	EXPECT_THROW((void)spk::DataVoxelShape(UniformSideSlots, std::move(twoVertices)), std::invalid_argument);

	spk::VoxelShapeDescription mismatchedUVs;
	mismatchedUVs.polygons = {{"top", {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}, {{0, 0}, {1, 0}}}};
	EXPECT_THROW((void)spk::DataVoxelShape(UniformSideSlots, std::move(mismatchedUVs)), std::invalid_argument);

	spk::VoxelShapeDescription outsideCell;
	outsideCell.polygons = {{"top", {{0, 1.5f, 1}, {1, 1.5f, 1}, {1, 1.5f, 0}, {0, 1.5f, 0}}, RectangleUVs}};
	EXPECT_THROW((void)spk::DataVoxelShape(UniformSideSlots, std::move(outsideCell)), std::invalid_argument);

	spk::VoxelShapeDescription unknownSlot;
	unknownSlot.polygons = {{"unknown", {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}, RectangleUVs}};
	EXPECT_THROW((void)spk::DataVoxelShape(UniformSideSlots, std::move(unknownSlot)), std::invalid_argument);
}

TEST(DataVoxelShape, BuriedDataSlabSuppressesItsTopFace)
{
	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	const spk::VoxelRuntimeId legacySlab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(spk::AtlasCell{0, 0}, 0.5f));
	const spk::VoxelRuntimeId dataSlab = registry.registerShape(std::make_unique<spk::DataVoxelShape>(UniformSideSlots, slabDescription()));

	// Same fully enclosed cell, two slab flavors: the legacy slab keeps its mid-height top
	// in the PositiveY outer slot, which the mesher can never occlude; the data slab's top
	// is an inner face, suppressed once every boundary plane is sealed by the cubes.
	spk::VoxelGrid legacyGrid({3, 3, 3}, {cube});
	legacyGrid.cell({1, 1, 1}) = {legacySlab};
	spk::VoxelGrid dataGrid({3, 3, 3}, {cube});
	dataGrid.cell({1, 1, 1}) = {dataSlab};

	const std::size_t legacyShapes = spk::VoxelMesher::buildRenderMesh(legacyGrid, registry).opaque.nbShape();
	const std::size_t dataShapes = spk::VoxelMesher::buildRenderMesh(dataGrid, registry).opaque.nbShape();
	EXPECT_EQ(dataShapes + 1u, legacyShapes);
}
