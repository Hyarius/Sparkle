#pragma once

#include "voxel/voxel_enums.hpp"

#include <stdexcept>

namespace pg
{
	// Per-orientation walk heights used by the navigation graph. This is gameplay data the
	// spk voxel library does not carry (it promoted render geometry only), so it lives here,
	// keyed by the numeric voxel id alongside the spk render shape.
	struct CardinalHeightSet
	{
		float positiveX = 0.0f;
		float negativeX = 0.0f;
		float positiveZ = 0.0f;
		float negativeZ = 0.0f;
		float stationary = 0.0f;

		[[nodiscard]] float get(VoxelOrientation p_direction) const
		{
			switch (p_direction)
			{
			case VoxelOrientation::PositiveX:
				return positiveX;
			case VoxelOrientation::PositiveZ:
				return positiveZ;
			case VoxelOrientation::NegativeX:
				return negativeX;
			case VoxelOrientation::NegativeZ:
				return negativeZ;
			}
			throw std::invalid_argument("Unknown voxel direction");
		}
	};

	struct CardinalHeightCollection
	{
		CardinalHeightSet positiveY;
		CardinalHeightSet negativeY;

		[[nodiscard]] const CardinalHeightSet &get(VoxelFlip p_flip) const noexcept
		{
			return p_flip == VoxelFlip::PositiveY ? positiveY : negativeY;
		}
	};
}
