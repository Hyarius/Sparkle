#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Full unit cube: six outer-shell faces, no inner face. Texture slots are
	// "posX", "negX", "top", "bottom", "posZ" and "negZ".
	class CubeVoxelShape : public spk::VoxelShape
	{
	protected:
		void _constructRenderFaces() override;

	public:
		explicit CubeVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit CubeVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
	};
}
