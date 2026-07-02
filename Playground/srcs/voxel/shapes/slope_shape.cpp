#include "voxel/shapes/slope_shape.hpp"

#include <utility>

namespace pg
{
	SlopeShape::SlopeShape(TextureSlots p_textures) :
		VoxelShape(std::move(p_textures))
	{
	}

	void SlopeShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		faces.innerFaces.push_back(createFace(createRectangle(
			"slope", {0, 0, 0}, {0, 1, 1}, {1, 1, 1}, {1, 0, 0})));
		faces.outer(VoxelAxisPlane::PositiveZ) = createFace(createRectangle(
			"back", {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}));
		faces.outer(VoxelAxisPlane::NegativeY) = createFace(createRectangle(
			"bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
		faces.outer(VoxelAxisPlane::PositiveX) = createFace(createTriangle(
			"sideRight", {1, 0, 1}, {1, 0, 0}, {1, 1, 1}));
		faces.outer(VoxelAxisPlane::NegativeX) = createFace(createTriangle(
			"sideLeft", {0, 0, 0}, {0, 0, 1}, {0, 1, 1}));
	}

	void SlopeShape::_constructMask()
	{
		mutableMaskFaces().positiveY.push_back(renderFaces().innerFaces.front());
		mutableMaskFaces().negativeY.push_back(*renderFaces().outer(VoxelAxisPlane::NegativeY));
	}

	CardinalHeightSet SlopeShape::_constructPositiveYHeights() const
	{
		return {0.5f, 0.5f, 1.0f, 0.0f, 0.5f};
	}

	CardinalHeightSet SlopeShape::_constructNegativeYHeights() const
	{
		return {1, 1, 1, 1, 1};
	}
}
