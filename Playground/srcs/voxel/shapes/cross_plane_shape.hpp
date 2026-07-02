#pragma once

#include "voxel/voxel_shape.hpp"

namespace pg
{
	class CrossPlaneShape : public VoxelShape
	{
	protected:
		void _constructRenderFaces() override;
		void _constructMask() override;
		[[nodiscard]] CardinalHeightSet _constructPositiveYHeights() const override;

	public:
		explicit CrossPlaneShape(TextureSlots p_textures);
	};
}
