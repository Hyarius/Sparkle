#pragma once

#include "board/board_data.hpp"

#include "structures/math/spk_vector2.hpp"

namespace pg
{
	class VoxelRegistry;
	class VoxelWorld;
	class VoxelGrid;

	class BoardBuilder
	{
	public:
		[[nodiscard]] static BoardData fromWorld(
			const VoxelWorld &p_world,
			const spk::Vector3Int &p_center,
			const spk::Vector2Int &p_size,
			int p_deploymentStripDepth = 2,
			std::size_t p_teamSize = 6,
			VoxelOrientation p_playerFacing = VoxelOrientation::PositiveZ,
			float p_maxVerticalGap = 0.5f);

		[[nodiscard]] static BoardData fromGrid(
			const VoxelGrid &p_grid,
			const VoxelRegistry &p_registry,
			int p_deploymentStripDepth = 2,
			VoxelOrientation p_playerFacing = VoxelOrientation::PositiveZ,
			float p_maxVerticalGap = 0.5f);
	};
}
