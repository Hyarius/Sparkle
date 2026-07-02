#pragma once

#include "voxel/voxel_shape.hpp"

namespace pg
{
	class SlabShape : public VoxelShape
	{
	private:
		float _height;

	protected:
		void _constructRenderFaces() override;
		void _constructMask() override;
		[[nodiscard]] CardinalHeightSet _constructPositiveYHeights() const override;
		[[nodiscard]] CardinalHeightSet _constructNegativeYHeights() const override;

	public:
		SlabShape(TextureSlots p_textures, float p_height = 0.5f);

		[[nodiscard]] float height() const noexcept;
	};
}
