#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Two double-sided, axis-aligned quads crossing at the cell center. This is
	// useful for vegetation and decorations that should form a + rather than the
	// diagonal X produced by DiagonalCrossVoxelShape. Texture slot is "plane".
	class CrossVoxelShape : public spk::VoxelShape
	{
	protected:
		void _constructRenderFaces() override;

	public:
		explicit CrossVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit CrossVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
	};
}
