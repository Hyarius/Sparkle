#pragma once

#include "structures/voxel/spk_voxel_enums.hpp"

#include <cstdint>

namespace spk
{
	struct VoxelCell
	{
		static constexpr std::int32_t EmptyId = -1;

		std::int32_t id = EmptyId;
		spk::VoxelOrientation orientation = spk::VoxelOrientation::PositiveZ;
		spk::VoxelFlip flip = spk::VoxelFlip::PositiveY;

		[[nodiscard]] bool isEmpty() const noexcept
		{
			return id == EmptyId;
		}

		[[nodiscard]] bool operator==(const VoxelCell &) const noexcept = default;
	};
}
