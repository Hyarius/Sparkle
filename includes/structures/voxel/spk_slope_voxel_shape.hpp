#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Ramp rising from the local -Z edge up to the +Z edge. The vertical back, the bottom
	// and the two triangular sides belong to the outer shell (only the back and bottom
	// are full quads able to occlude a neighbor); the slanted quad is an inner face.
	// Texture slots are "slope", "back", "bottom", "sideRight" and "sideLeft".
	class SlopeVoxelShape : public spk::VoxelShape
	{
	protected:
		void _constructRenderFaces() override;

	public:
		explicit SlopeVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit SlopeVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
	};
}
