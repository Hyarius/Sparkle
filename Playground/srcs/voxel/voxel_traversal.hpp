#pragma once

#include "voxel/voxel_grid.hpp"
#include "voxel/voxel_registry.hpp"
#include "voxel/voxel_traversal_data.hpp"

#include "structures/math/spk_vector3.hpp"

namespace pg
{
	[[nodiscard]] bool isSolid(const VoxelCell &p_cell, const VoxelRegistry &p_registry);
	[[nodiscard]] bool isPassableSpace(const VoxelCell &p_cell, const VoxelRegistry &p_registry);
	[[nodiscard]] bool isStandable(
		const VoxelGrid &p_grid,
		const spk::Vector3Int &p_position,
		const VoxelRegistry &p_registry);

	[[nodiscard]] VoxelOrientation resolveLocalDirection(
		VoxelOrientation p_worldDirection,
		VoxelOrientation p_orientation);
	[[nodiscard]] CardinalHeightSet resolveWorldHeights(
		const CardinalHeightSet &p_localHeights,
		VoxelOrientation p_orientation);
}
