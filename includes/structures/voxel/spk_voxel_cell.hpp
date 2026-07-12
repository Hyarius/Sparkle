#pragma once

#include "structures/voxel/spk_voxel_enums.hpp"
#include "structures/voxel/spk_voxel_ids.hpp"

namespace spk
{
	// One cell of a voxel grid: the dense runtime handle of the registered voxel state it
	// holds (see spk::VoxelRegistry), plus the transformations applied to that state.
	// Orientation and flip are transformations of the current state, not states themselves:
	// a state describes an actual semantic variation (texture variant, fill level, ...).
	struct VoxelCell
	{
		spk::VoxelRuntimeId id{};
		spk::VoxelOrientation orientation = spk::VoxelOrientation::PositiveZ;
		spk::VoxelFlip flip = spk::VoxelFlip::PositiveY;

		[[nodiscard]] bool isEmpty() const noexcept
		{
			return !id.isValid();
		}

		[[nodiscard]] bool operator==(const VoxelCell &) const noexcept = default;
	};
}
