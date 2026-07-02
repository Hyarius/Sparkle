#pragma once

#include "voxel/voxel_grid.hpp"

namespace pg
{
	class VoxelRegistry;

	// The no-argument form uses the stable sorted ids guaranteed by VoxelRegistry for the
	// eight authored Step 4 definitions. Runtime code should prefer the validated overload.
	[[nodiscard]] VoxelGrid buildShowcaseGrid();
	[[nodiscard]] VoxelGrid buildShowcaseGrid(const VoxelRegistry &p_registry);
}
