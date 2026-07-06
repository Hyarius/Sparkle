#pragma once

#include "structures/voxel/spk_voxel_grid.hpp"
#include "voxel/voxel_cell.hpp"

namespace pg
{
	// The Playground grid was promoted verbatim into spk::VoxelGrid; alias it so prefab grids,
	// content parsing and generators all operate on the same grid the spk voxel chunks own.
	using VoxelGrid = spk::VoxelGrid;
}
