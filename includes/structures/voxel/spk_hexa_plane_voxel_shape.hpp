#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Six double-sided, axis-aligned quads inset into the voxel cell: two parallel
	// planes for each axis. This gives foliage volume without closing it into a cube.
	// Texture slot is "plane".
	class HexaPlaneVoxelShape : public spk::VoxelShape
	{
	protected:
		void _constructRenderFaces() override;

	public:
		explicit HexaPlaneVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit HexaPlaneVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
	};
}
