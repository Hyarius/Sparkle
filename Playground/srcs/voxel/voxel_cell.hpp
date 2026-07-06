#pragma once

#include "structures/voxel/spk_voxel_cell.hpp"
#include "voxel/voxel_enums.hpp"

namespace pg
{
	// spk::VoxelCell is identical to the historical pg::VoxelCell (id + orientation + flip);
	// alias it so the app and the spk voxel map share one cell type.
	using VoxelCell = spk::VoxelCell;
}
