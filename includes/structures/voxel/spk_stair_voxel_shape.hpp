#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Staircase climbing from the local -Z edge up to the +Z edge. The vertical back, the
	// bottom and the two stepped side faces belong to the outer shell (only the back
	// and bottom are full quads able to occlude a neighbor); the step tops and risers are
	// inner faces. Texture slots are "top", "riser", "back", "bottom", "sideRight" and
	// "sideLeft".
	class StairVoxelShape : public spk::VoxelShape
	{
	private:
		int _stepCount;

		[[nodiscard]] spk::VoxelShapeFace _constructSideFace(bool p_positiveX) const;

	protected:
		void _constructRenderFaces() override;

	public:
		explicit StairVoxelShape(TextureSlots p_textures, int p_stepCount = 2, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		explicit StairVoxelShape(const spk::AtlasCell &p_uniformTexture, int p_stepCount = 2, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);

		[[nodiscard]] int stepCount() const noexcept;
	};
}
