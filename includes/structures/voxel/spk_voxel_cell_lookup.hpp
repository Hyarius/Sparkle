#pragma once

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_cell.hpp"

namespace spk
{
	// Read access to voxel cells in world coordinates. Returned pointers only need
	// to remain readable until the lookup call returns; consumers copy values they
	// need to retain.
	class IVoxelCellLookup
	{
	public:
		virtual ~IVoxelCellLookup() = default;

		[[nodiscard]] virtual const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const = 0;

		// Rendering can optionally expose a filtered view while other spatial queries
		// continue to use the complete cell set.
		[[nodiscard]] virtual const spk::VoxelCell *tryRenderableCell(const spk::Vector3Int &p_worldCell) const
		{
			return tryCell(p_worldCell);
		}
	};
}
