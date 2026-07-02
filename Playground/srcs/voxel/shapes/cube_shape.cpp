#include "voxel/shapes/cube_shape.hpp"

#include <utility>

namespace pg
{
	CubeShape::CubeShape(TextureSlots p_textures) :
		VoxelShape(std::move(p_textures))
	{
	}

	void CubeShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		faces.outer(VoxelAxisPlane::PositiveX) = createFace(createRectangle(
			"posX", {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}));
		faces.outer(VoxelAxisPlane::NegativeX) = createFace(createRectangle(
			"negX", {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}));
		faces.outer(VoxelAxisPlane::PositiveY) = createFace(createRectangle(
			"top", {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}));
		faces.outer(VoxelAxisPlane::NegativeY) = createFace(createRectangle(
			"bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
		faces.outer(VoxelAxisPlane::PositiveZ) = createFace(createRectangle(
			"posZ", {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}));
		faces.outer(VoxelAxisPlane::NegativeZ) = createFace(createRectangle(
			"negZ", {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}));
	}

	void CubeShape::_constructMask()
	{
		mutableMaskFaces().positiveY.push_back(*renderFaces().outer(VoxelAxisPlane::PositiveY));
		mutableMaskFaces().negativeY.push_back(*renderFaces().outer(VoxelAxisPlane::NegativeY));
	}

	CardinalHeightSet CubeShape::_constructPositiveYHeights() const
	{
		return {1, 1, 1, 1, 1};
	}
}
