#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"
#include "structures/voxel/spk_cross_voxel_shape.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_hexa_plane_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"

namespace
{
	struct ShapeLibrary
	{
		spk::VoxelRegistry registry;
		std::int32_t cube = 0;
		std::int32_t halfSlab = 0;
		std::int32_t fullSlab = 0;
		std::int32_t slope = 0;
		std::int32_t stair = 0;
		std::int32_t cross = 0;

		ShapeLibrary()
		{
			const spk::AtlasCell texture{0, 0};
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(texture));
			halfSlab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(texture));
			fullSlab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(texture, 1.0f));
			slope = registry.registerShape(std::make_unique<spk::SlopeVoxelShape>(texture));
			stair = registry.registerShape(std::make_unique<spk::StairVoxelShape>(texture));
			cross = registry.registerShape(std::make_unique<spk::DiagonalCrossVoxelShape>(texture));
		}
	};
}

TEST(VoxelShapeLibrary, ShapesSplitTheirFacesBetweenTheTwoShells)
{
	const ShapeLibrary library;

	// A slab's sides lie on the cell boundary, so all six faces are outer-shell faces:
	// neighbors can occlude the sides (partial coverage) even though they never occlude.
	const spk::VoxelShapeFaceSet &slabFaces = library.registry.shape(library.halfSlab).renderFaces();
	EXPECT_EQ(slabFaces.outerFaceCount(), 6u);
	EXPECT_EQ(slabFaces.innerFaces.size(), 0u);

	const spk::VoxelShapeFaceSet &slopeFaces = library.registry.shape(library.slope).renderFaces();
	EXPECT_EQ(slopeFaces.outerFaceCount(), 4u);
	EXPECT_EQ(slopeFaces.innerFaces.size(), 1u);

	const spk::VoxelShapeFaceSet &stairFaces = library.registry.shape(library.stair).renderFaces();
	EXPECT_EQ(stairFaces.outerFaceCount(), 4u);
	EXPECT_EQ(stairFaces.innerFaces.size(), 4u);

	const spk::VoxelShapeFaceSet &crossFaces = library.registry.shape(library.cross).renderFaces();
	EXPECT_EQ(crossFaces.outerFaceCount(), 0u);
	EXPECT_EQ(crossFaces.innerFaces.size(), 4u);
}

TEST(VoxelShapeLibrary, ParametersAreValidated)
{
	const spk::AtlasCell texture{0, 0};

	EXPECT_THROW(spk::SlabVoxelShape(texture, -0.1f), std::invalid_argument);
	EXPECT_THROW(spk::SlabVoxelShape(texture, 1.5f), std::invalid_argument);
	EXPECT_FLOAT_EQ(spk::SlabVoxelShape(texture).height(), 0.5f);

	EXPECT_THROW(spk::StairVoxelShape(texture, 0), std::invalid_argument);
	EXPECT_THROW(spk::StairVoxelShape(texture, -2), std::invalid_argument);
	EXPECT_EQ(spk::StairVoxelShape(texture).stepCount(), 2);
	EXPECT_EQ(spk::StairVoxelShape(texture, 4).stepCount(), 4);
}

TEST(VoxelShapeLibrary, CubeKeepsTopUVsAndFlipsVerticalFaceV)
{
	spk::CubeVoxelShape shape(spk::AtlasCell{0, 0}, spk::Vector2Int{1, 1});
	shape.initialize();

	const auto &top = shape.renderFaces().outer(spk::VoxelAxisPlane::PositiveY)->polygons().front();
	EXPECT_EQ(top[0].data, spk::Vector2(0, 0));
	EXPECT_EQ(top[1].data, spk::Vector2(1, 0));
	EXPECT_EQ(top[2].data, spk::Vector2(1, 1));
	EXPECT_EQ(top[3].data, spk::Vector2(0, 1));

	const auto &side = shape.renderFaces().outer(spk::VoxelAxisPlane::PositiveZ)->polygons().front();
	EXPECT_EQ(side[0].data, spk::Vector2(0, 1));
	EXPECT_EQ(side[1].data, spk::Vector2(1, 1));
	EXPECT_EQ(side[2].data, spk::Vector2(1, 0));
	EXPECT_EQ(side[3].data, spk::Vector2(0, 0));
}

TEST(VoxelShapeLibrary, SlopeTextureRunsAcrossXAndUpTheSlope)
{
	spk::SlopeVoxelShape shape(spk::AtlasCell{0, 0}, spk::Vector2Int{1, 1});
	shape.initialize();

	const auto &slope = shape.renderFaces().innerFaces.front().polygons().front();
	EXPECT_EQ(slope[0].data, spk::Vector2(0, 1));
	EXPECT_EQ(slope[1].data, spk::Vector2(0, 0));
	EXPECT_EQ(slope[2].data, spk::Vector2(1, 0));
	EXPECT_EQ(slope[3].data, spk::Vector2(1, 1));
}

TEST(VoxelShapeLibrary, HalfSlabSidesUseTheLowerHalfOfTheirTexture)
{
	spk::SlabVoxelShape shape(spk::AtlasCell{0, 0}, 0.5f, spk::Vector2Int{1, 1});
	shape.initialize();

	for (const auto plane : {spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::NegativeZ})
	{
		const auto &side = shape.renderFaces().outer(plane)->polygons().front();
		EXPECT_EQ(side[0].data.y, 1.0f);
		EXPECT_EQ(side[1].data.y, 1.0f);
		EXPECT_EQ(side[2].data.y, 0.5f);
		EXPECT_EQ(side[3].data.y, 0.5f);
	}
}

TEST(VoxelShapeLibrary, StairTreadsAndRisersUseTheirTextureBands)
{
	spk::StairVoxelShape shape(spk::AtlasCell{0, 0}, 2, spk::Vector2Int{1, 1});
	shape.initialize();

	const auto &faces = shape.renderFaces().innerFaces;
	ASSERT_EQ(faces.size(), 4u);

	const auto &lowerTop = faces[0].polygons().front();
	EXPECT_EQ(lowerTop[0].data.y, 0.5f);
	EXPECT_EQ(lowerTop[2].data.y, 1.0f);
	const auto &lowerRiser = faces[1].polygons().front();
	EXPECT_EQ(lowerRiser[0].data.y, 1.0f);
	EXPECT_EQ(lowerRiser[2].data.y, 0.5f);

	const auto &upperTop = faces[2].polygons().front();
	EXPECT_EQ(upperTop[0].data.y, 0.0f);
	EXPECT_EQ(upperTop[2].data.y, 0.5f);
	const auto &upperRiser = faces[3].polygons().front();
	EXPECT_EQ(upperRiser[0].data.y, 0.5f);
	EXPECT_EQ(upperRiser[2].data.y, 0.0f);
}

TEST(VoxelShapeLibrary, StairSidesAreSplitIntoConvexStepQuads)
{
	spk::StairVoxelShape shape(spk::AtlasCell{0, 0}, 3);
	shape.initialize();

	for (const spk::VoxelAxisPlane plane : {spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeX})
	{
		const auto &side = shape.renderFaces().outer(plane);
		ASSERT_TRUE(side.has_value());
		ASSERT_EQ(side->size(), 3u);
		for (const auto &step : side->polygons())
		{
			EXPECT_EQ(step.size(), 4u);
		}
	}
}

TEST(VoxelShapeLibrary, FullHeightSlabOccludesLikeACubeButHalfSlabDoesNot)
{
	const ShapeLibrary library;

	spk::VoxelGrid full({1, 2, 1});
	full.cell(0, 0, 0) = {library.fullSlab};
	full.cell(0, 1, 0) = {library.cube};
	// Both faces of the shared horizontal boundary are culled.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(full, library.registry).opaque.nbShape(), 10u);

	spk::VoxelGrid half({1, 2, 1});
	half.cell(0, 0, 0) = {library.halfSlab};
	half.cell(0, 1, 0) = {library.cube};
	// The half slab's top sits at mid-height: nothing is culled on either side.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(half, library.registry).opaque.nbShape(), 12u);
}

TEST(VoxelShapeLibrary, SlopeBackOccludesButTriangleSideDoesNot)
{
	const ShapeLibrary library;

	spk::VoxelGrid behind({1, 1, 2});
	behind.cell(0, 0, 0) = {library.slope};
	behind.cell(0, 0, 1) = {library.cube};
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(behind, library.registry).opaque.nbShape(), 9u);

	spk::VoxelGrid beside({2, 1, 1});
	beside.cell(0, 0, 0) = {library.slope};
	beside.cell(1, 0, 0) = {library.cube};
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(beside, library.registry).opaque.nbShape(), 10u);
}

TEST(VoxelShapeLibrary, RotatedSlopeOccludesTheCorrectCubeFace)
{
	const ShapeLibrary library;

	// The slope's back (local +Z) faces world -Z once oriented NegativeZ: the cube placed
	// behind it loses its +Z face and the slope loses its back.
	spk::VoxelGrid grid({1, 1, 2});
	grid.cell(0, 0, 0) = {library.cube};
	grid.cell(0, 0, 1) = {library.slope, spk::VoxelOrientation::NegativeZ};
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, library.registry).opaque.nbShape(), 9u);
}

TEST(VoxelShapeLibrary, StairBackOccludesButSteppedSideDoesNot)
{
	const ShapeLibrary library;

	spk::VoxelGrid behind({1, 1, 2});
	behind.cell(0, 0, 0) = {library.stair};
	behind.cell(0, 0, 1) = {library.cube};
	// Stair keeps bottom + four convex side quads + 4 inner step faces; cube keeps 5 faces.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(behind, library.registry).opaque.nbShape(), 14u);

	spk::VoxelGrid beside({2, 1, 1});
	beside.cell(0, 0, 0) = {library.stair};
	beside.cell(1, 0, 0) = {library.cube};
	// The stair's +X stepped polygon is culled by the cube, but its own silhouette is not
	// a full quad so the cube keeps all six faces.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(beside, library.registry).opaque.nbShape(), 14u);
}

TEST(VoxelShapeLibrary, CrossPlaneNeverOccludesOrGetsOccluded)
{
	const ShapeLibrary library;

	spk::VoxelGrid lone({1, 1, 1}, {library.cross});
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(lone, library.registry).opaque.nbShape(), 4u);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {library.cross};
	grid.cell(1, 0, 0) = {library.cube};
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, library.registry).opaque.nbShape(), 10u);
}

TEST(VoxelShapeLibrary, CrossUsesTwoCenteredAxisAlignedPlanes)
{
	spk::CrossVoxelShape shape(spk::AtlasCell{0, 0});
	shape.initialize();

	const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
	EXPECT_EQ(faces.outerFaceCount(), 0u);
	ASSERT_EQ(faces.innerFaces.size(), 4u);

	const auto &alongX = faces.innerFaces[0].polygons().front();
	const auto &alongZ = faces.innerFaces[2].polygons().front();
	for (const spk::VoxelShapeVertex &vertex : alongX)
	{
		EXPECT_FLOAT_EQ(vertex.position.z, 0.5f);
	}
	for (const spk::VoxelShapeVertex &vertex : alongZ)
	{
		EXPECT_FLOAT_EQ(vertex.position.x, 0.5f);
	}
}

TEST(VoxelShapeLibrary, hexaPlaneUsesTwoPlanesOnEachAxis)
{
	spk::HexaPlaneVoxelShape shape(spk::AtlasCell{0, 0});
	shape.initialize();

	const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
	EXPECT_EQ(faces.outerFaceCount(), 0u);
	ASSERT_EQ(faces.innerFaces.size(), 12u);

	std::array<int, 2> xPlanes{};
	std::array<int, 2> yPlanes{};
	std::array<int, 2> zPlanes{};
	for (std::size_t index = 0; index < faces.innerFaces.size(); index += 2)
	{
		const auto &polygon = faces.innerFaces[index].polygons().front();
		const auto countPlane = [&](auto member, std::array<int, 2> &counts) {
			const float coordinate = polygon[0].position.*member;
			for (const auto &vertex : polygon)
				if (vertex.position.*member != coordinate)
					return false;
			++counts[coordinate < 0.5f ? 0 : 1];
			return true;
		};
		EXPECT_TRUE(countPlane(&spk::Vector3::x, xPlanes) || countPlane(&spk::Vector3::y, yPlanes) ||
			countPlane(&spk::Vector3::z, zPlanes));
	}
	EXPECT_EQ(xPlanes, (std::array<int, 2>{1, 1}));
	EXPECT_EQ(yPlanes, (std::array<int, 2>{1, 1}));
	EXPECT_EQ(zPlanes, (std::array<int, 2>{1, 1}));
}
