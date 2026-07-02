#pragma once

#include "voxel/voxel_shape.hpp"

namespace pg
{
	class SlopeShape : public VoxelShape
	{
	protected:
		void _constructRenderFaces() override;
		void _constructMask() override;
		[[nodiscard]] CardinalHeightSet _constructPositiveYHeights() const override;
		[[nodiscard]] CardinalHeightSet _constructNegativeYHeights() const override;

	public:
		explicit SlopeShape(TextureSlots p_textures);
	};
}
