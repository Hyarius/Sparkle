#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"

#include <utility>

namespace spk
{
	CrossPlaneVoxelShape::CrossPlaneVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize)
	{
	}

	CrossPlaneVoxelShape::CrossPlaneVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize) :
		CrossPlaneVoxelShape(TextureSlots{{"plane", p_uniformTexture}}, p_atlasSize)
	{
	}

	void CrossPlaneVoxelShape::_constructRenderFaces()
	{
		const spk::VoxelShapePolygon first = createVerticalRectangle(
			"plane", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0});
		const spk::VoxelShapePolygon second = createVerticalRectangle(
			"plane", {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, {1, 1, 0});

		mutableRenderFaces().innerFaces.push_back(createFace(first));
		mutableRenderFaces().innerFaces.push_back(createFace(spk::VoxelShapePolygon(first.rbegin(), first.rend())));
		mutableRenderFaces().innerFaces.push_back(createFace(second));
		mutableRenderFaces().innerFaces.push_back(createFace(spk::VoxelShapePolygon(second.rbegin(), second.rend())));
	}
}
