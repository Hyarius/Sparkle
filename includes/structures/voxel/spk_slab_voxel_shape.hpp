#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Horizontal slice of a cube, from the floor up to the requested height. The top and
	// bottom belong to the outer shell (a full-height slab therefore occludes like a
	// cube); the four partial side quads are inner faces. Texture slots are "top",
	// "bottom", "posX", "negX", "posZ" and "negZ".
	class SlabVoxelShape : public spk::VoxelShape
	{
	private:
		float _height;

	protected:
		void _constructRenderFaces() override;

	public:
		explicit SlabVoxelShape(TextureSlots p_textures, float p_height = 0.5f, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit SlabVoxelShape(const spk::AtlasCell &p_uniformTexture, float p_height = 0.5f, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);

		[[nodiscard]] float height() const noexcept;
	};
}
