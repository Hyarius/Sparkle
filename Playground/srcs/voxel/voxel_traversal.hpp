#pragma once

#include "voxel/voxel_registry.hpp"
#include "voxel/voxel_traversal_data.hpp"

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"

namespace pg
{
	[[nodiscard]] bool isSolid(const spk::VoxelCell &p_cell, const VoxelRegistry &p_registry);
	[[nodiscard]] bool isPassableSpace(const spk::VoxelCell &p_cell, const VoxelRegistry &p_registry);
	[[nodiscard]] bool isStandable(
		const spk::VoxelGrid &p_grid,
		const spk::Vector3Int &p_position,
		const VoxelRegistry &p_registry);

	[[nodiscard]] VoxelOrientation resolveLocalDirection(
		VoxelOrientation p_worldDirection,
		VoxelOrientation p_orientation);
	[[nodiscard]] CardinalHeightSet resolveWorldHeights(
		const CardinalHeightSet &p_localHeights,
		VoxelOrientation p_orientation);
}
