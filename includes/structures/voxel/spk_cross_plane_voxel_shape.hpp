#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Two double-sided diagonal quads crossing at the cell center (vegetation, decors...).
	// Everything is an inner face: the shape never occludes a neighbor and is never
	// culled while visible. Texture slot is "plane".
	class CrossPlaneVoxelShape : public spk::VoxelShape
	{
	protected:
		void _constructRenderFaces() override;

	public:
		explicit CrossPlaneVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit CrossPlaneVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
	};
}
