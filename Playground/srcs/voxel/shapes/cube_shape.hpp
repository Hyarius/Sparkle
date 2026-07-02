#pragma once

#include "voxel/voxel_shape.hpp"

namespace pg
{
	class CubeShape : public VoxelShape
	{
	protected:
		void _constructRenderFaces() override;
		void _constructMask() override;
		[[nodiscard]] CardinalHeightSet _constructPositiveYHeights() const override;

	public:
		explicit CubeShape(TextureSlots p_textures);
	};
}
