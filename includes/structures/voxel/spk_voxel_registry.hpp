#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace spk
{
	// Maps the numeric id stored in a VoxelCell to the shape describing its geometry.
	// Shapes are initialized on registration; ids are attributed sequentially from 0.
	class VoxelRegistry
	{
	private:
		std::vector<std::unique_ptr<spk::VoxelShape>> _shapes;

	public:
		std::int32_t registerShape(std::unique_ptr<spk::VoxelShape> p_shape);

		[[nodiscard]] const spk::VoxelShape &shape(std::int32_t p_id) const;
		[[nodiscard]] const spk::VoxelShape *tryShape(std::int32_t p_id) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};
}
