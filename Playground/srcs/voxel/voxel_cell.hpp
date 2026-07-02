#pragma once

#include "voxel/voxel_enums.hpp"

#include <cstdint>

namespace pg
{
	struct VoxelCell
	{
		static constexpr std::int32_t EmptyId = -1;

		std::int32_t id = EmptyId;
		VoxelOrientation orientation = VoxelOrientation::PositiveZ;
		VoxelFlip flip = VoxelFlip::PositiveY;

		[[nodiscard]] bool isEmpty() const noexcept
		{
			return id == EmptyId;
		}
	};
}
