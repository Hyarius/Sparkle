#pragma once

#include "voxel/voxel_shape.hpp"

namespace pg
{
	class StairShape : public VoxelShape
	{
	private:
		int _stepCount;

		[[nodiscard]] VoxelShapeFace _constructSideFace(bool p_positiveX) const;

	protected:
		void _constructRenderFaces() override;
		void _constructMask() override;
		[[nodiscard]] CardinalHeightSet _constructPositiveYHeights() const override;
		[[nodiscard]] CardinalHeightSet _constructNegativeYHeights() const override;

	public:
		StairShape(TextureSlots p_textures, int p_stepCount = 2);

		[[nodiscard]] int stepCount() const noexcept;
	};
}
