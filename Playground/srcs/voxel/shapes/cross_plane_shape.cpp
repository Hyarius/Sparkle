#include "voxel/shapes/cross_plane_shape.hpp"

#include <utility>

namespace pg
{
	CrossPlaneShape::CrossPlaneShape(TextureSlots p_textures) :
		VoxelShape(std::move(p_textures))
	{
	}

	void CrossPlaneShape::_constructRenderFaces()
	{
		const VoxelShapePolygon first = createRectangle(
			"plane", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0});
		const VoxelShapePolygon second = createRectangle(
			"plane", {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, {1, 1, 0});

		mutableRenderFaces().innerFaces.push_back(createFace(first));
		mutableRenderFaces().innerFaces.push_back(createFace(VoxelShapePolygon(first.rbegin(), first.rend())));
		mutableRenderFaces().innerFaces.push_back(createFace(second));
		mutableRenderFaces().innerFaces.push_back(createFace(VoxelShapePolygon(second.rbegin(), second.rend())));
	}

	void CrossPlaneShape::_constructMask()
	{
	}

	CardinalHeightSet CrossPlaneShape::_constructPositiveYHeights() const
	{
		return {};
	}
}
