#include "voxel/shapes/cross_plane_shape.hpp"
#include "voxel/shapes/cube_shape.hpp"
#include "voxel/shapes/slab_shape.hpp"
#include "voxel/shapes/slope_shape.hpp"
#include "voxel/shapes/stair_shape.hpp"

#include <gtest/gtest.h>

namespace
{
	pg::VoxelShape::TextureSlots cubeTextures()
	{
		return {
			{"top", {0, 0}},
			{"bottom", {1, 0}},
			{"posX", {2, 0}},
			{"negX", {3, 0}},
			{"posZ", {4, 0}},
			{"negZ", {5, 0}}};
	}

	pg::VoxelShape::TextureSlots slopeTextures()
	{
		return {
			{"slope", {0, 1}},
			{"back", {1, 1}},
			{"bottom", {2, 1}},
			{"sideLeft", {3, 1}},
			{"sideRight", {4, 1}}};
	}

	pg::VoxelShape::TextureSlots stairTextures()
	{
		return {
			{"top", {0, 2}},
			{"riser", {1, 2}},
			{"back", {2, 2}},
			{"bottom", {3, 2}},
			{"sideLeft", {4, 2}},
			{"sideRight", {5, 2}}};
	}

	void expectAllHeights(const pg::CardinalHeightSet &p_heights, float p_expected)
	{
		EXPECT_FLOAT_EQ(p_heights.positiveX, p_expected);
		EXPECT_FLOAT_EQ(p_heights.negativeX, p_expected);
		EXPECT_FLOAT_EQ(p_heights.positiveZ, p_expected);
		EXPECT_FLOAT_EQ(p_heights.negativeZ, p_expected);
		EXPECT_FLOAT_EQ(p_heights.stationary, p_expected);
	}
}

TEST(VoxelShape, AtlasUVMapsUniformEightByEightGrid)
{
	EXPECT_EQ(pg::VoxelShape::atlasUV({2, 3}, {0.0f, 0.0f}), spk::Vector2(0.25f, 0.375f));
	EXPECT_EQ(pg::VoxelShape::atlasUV({2, 3}, {1.0f, 0.5f}), spk::Vector2(0.375f, 0.4375f));
}

TEST(CubeShape, BuildsShellMasksAndUnitHeights)
{
	pg::CubeShape shape(cubeTextures());
	shape.initialize();

	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 6);
	EXPECT_TRUE(shape.renderFaces().innerFaces.empty());
	EXPECT_EQ(shape.maskFaces().positiveY.size(), 1);
	EXPECT_EQ(shape.maskFaces().negativeY.size(), 1);
	expectAllHeights(shape.heights(pg::VoxelFlip::PositiveY), 1.0f);
	expectAllHeights(shape.heights(pg::VoxelFlip::NegativeY), 1.0f);

	const auto &positiveX = shape.renderFaces().outer(pg::VoxelAxisPlane::PositiveX).value();
	ASSERT_EQ(positiveX.polygons.front().size(), 4);
	EXPECT_EQ(positiveX.polygons.front()[0].position, spk::Vector3(1, 0, 1));
}

TEST(SlabShape, BuildsNonOccludableSidesAndFlipHeights)
{
	pg::SlabShape shape(cubeTextures(), 0.25f);
	shape.initialize();

	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 2);
	EXPECT_EQ(shape.renderFaces().innerFaces.size(), 4);
	EXPECT_FLOAT_EQ(shape.renderFaces().outer(pg::VoxelAxisPlane::PositiveY)->polygons[0][0].position.y, 0.25f);
	expectAllHeights(shape.heights(pg::VoxelFlip::PositiveY), 0.25f);
	expectAllHeights(shape.heights(pg::VoxelFlip::NegativeY), 1.0f);
}

TEST(SlopeShape, BuildsWedgeAndAsymmetricHeightTables)
{
	pg::SlopeShape shape(slopeTextures());
	shape.initialize();

	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 4);
	ASSERT_EQ(shape.renderFaces().innerFaces.size(), 1);
	EXPECT_EQ(shape.renderFaces().innerFaces[0].polygons[0][0].position, spk::Vector3(0, 0, 0));
	EXPECT_EQ(shape.renderFaces().innerFaces[0].polygons[0][1].position, spk::Vector3(0, 1, 1));

	const pg::CardinalHeightSet &positive = shape.heights(pg::VoxelFlip::PositiveY);
	EXPECT_FLOAT_EQ(positive.positiveX, 0.5f);
	EXPECT_FLOAT_EQ(positive.negativeX, 0.5f);
	EXPECT_FLOAT_EQ(positive.positiveZ, 1.0f);
	EXPECT_FLOAT_EQ(positive.negativeZ, 0.0f);
	EXPECT_FLOAT_EQ(positive.stationary, 0.5f);
	expectAllHeights(shape.heights(pg::VoxelFlip::NegativeY), 1.0f);
}

TEST(StairShape, BuildsTreadsRisersProfilesAndHeightTables)
{
	pg::StairShape shape(stairTextures(), 2);
	shape.initialize();

	EXPECT_EQ(shape.renderFaces().innerFaces.size(), 4);
	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 4);
	EXPECT_EQ(shape.maskFaces().positiveY.size(), 2);
	EXPECT_EQ(shape.maskFaces().negativeY.size(), 1);
	EXPECT_EQ(shape.renderFaces().innerFaces[0].polygons[0][0].position, spk::Vector3(0, 0.5f, 0.5f));

	const pg::CardinalHeightSet &positive = shape.heights(pg::VoxelFlip::PositiveY);
	EXPECT_FLOAT_EQ(positive.positiveZ, 1.0f);
	EXPECT_FLOAT_EQ(positive.negativeZ, 0.25f);
	EXPECT_FLOAT_EQ(positive.stationary, 0.5f);
	expectAllHeights(shape.heights(pg::VoxelFlip::NegativeY), 1.0f);
}

TEST(CrossPlaneShape, BuildsDoubleSidedInnerPlanesWithoutShellOrMasks)
{
	pg::CrossPlaneShape shape({{"plane", {0, 3}}});
	shape.initialize();

	EXPECT_EQ(shape.renderFaces().innerFaces.size(), 4);
	EXPECT_EQ(shape.renderFaces().outerFaceCount(), 0);
	EXPECT_TRUE(shape.maskFaces().positiveY.empty());
	EXPECT_TRUE(shape.maskFaces().negativeY.empty());
	expectAllHeights(shape.heights(pg::VoxelFlip::PositiveY), 0.0f);
	expectAllHeights(shape.heights(pg::VoxelFlip::NegativeY), 0.0f);
}
