#include "voxel/shapes/slab_shape.hpp"

#include <stdexcept>
#include <utility>

namespace pg
{
	SlabShape::SlabShape(TextureSlots p_textures, float p_height) :
		VoxelShape(std::move(p_textures)),
		_height(p_height)
	{
		if (_height < 0.0f || _height > 1.0f)
		{
			throw std::invalid_argument("Slab height must be between 0 and 1");
		}
	}

	void SlabShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		faces.outer(VoxelAxisPlane::PositiveY) = createFace(createRectangle(
			"top", {0, _height, 1}, {1, _height, 1}, {1, _height, 0}, {0, _height, 0}));
		faces.outer(VoxelAxisPlane::NegativeY) = createFace(createRectangle(
			"bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));

		faces.innerFaces.push_back(createFace(createRectangle(
			"posX", {1, 0, 1}, {1, 0, 0}, {1, _height, 0}, {1, _height, 1})));
		faces.innerFaces.push_back(createFace(createRectangle(
			"negX", {0, 0, 0}, {0, 0, 1}, {0, _height, 1}, {0, _height, 0})));
		faces.innerFaces.push_back(createFace(createRectangle(
			"posZ", {0, 0, 1}, {1, 0, 1}, {1, _height, 1}, {0, _height, 1})));
		faces.innerFaces.push_back(createFace(createRectangle(
			"negZ", {1, 0, 0}, {0, 0, 0}, {0, _height, 0}, {1, _height, 0})));
	}

	void SlabShape::_constructMask()
	{
		mutableMaskFaces().positiveY.push_back(*renderFaces().outer(VoxelAxisPlane::PositiveY));
		mutableMaskFaces().negativeY.push_back(*renderFaces().outer(VoxelAxisPlane::NegativeY));
	}

	CardinalHeightSet SlabShape::_constructPositiveYHeights() const
	{
		return {_height, _height, _height, _height, _height};
	}

	CardinalHeightSet SlabShape::_constructNegativeYHeights() const
	{
		return {1, 1, 1, 1, 1};
	}

	float SlabShape::height() const noexcept
	{
		return _height;
	}
}
