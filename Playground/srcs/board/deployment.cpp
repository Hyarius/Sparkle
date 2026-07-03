#include "board/deployment.hpp"

#include "board/board_data.hpp"

#include <algorithm>

namespace
{
	[[nodiscard]] bool inPlayerStrip(
		const spk::Vector3Int &p_cell,
		const spk::Vector3Int &p_size,
		int p_depth,
		pg::VoxelOrientation p_facing)
	{
		switch (p_facing)
		{
		case pg::VoxelOrientation::PositiveX:
			return p_cell.x >= p_size.x - p_depth;
		case pg::VoxelOrientation::NegativeX:
			return p_cell.x < p_depth;
		case pg::VoxelOrientation::PositiveZ:
			return p_cell.z >= p_size.z - p_depth;
		case pg::VoxelOrientation::NegativeZ:
			return p_cell.z < p_depth;
		}
		return false;
	}

	[[nodiscard]] bool inEnemyStrip(
		const spk::Vector3Int &p_cell,
		const spk::Vector3Int &p_size,
		int p_depth,
		pg::VoxelOrientation p_facing)
	{
		switch (p_facing)
		{
		case pg::VoxelOrientation::PositiveX:
			return p_cell.x < p_depth;
		case pg::VoxelOrientation::NegativeX:
			return p_cell.x >= p_size.x - p_depth;
		case pg::VoxelOrientation::PositiveZ:
			return p_cell.z < p_depth;
		case pg::VoxelOrientation::NegativeZ:
			return p_cell.z >= p_size.z - p_depth;
		}
		return false;
	}
}

namespace pg
{
	DeploymentZones Deployment::compute(
		const BoardData &p_board,
		int p_stripDepth,
		VoxelOrientation p_playerFacing)
	{
		DeploymentZones result;
		if (p_stripDepth <= 0)
		{
			return result;
		}
		const spk::Vector3Int size = p_board.terrain().size();
		const bool xAxis = p_playerFacing == VoxelOrientation::PositiveX ||
						   p_playerFacing == VoxelOrientation::NegativeX;
		const int extent = xAxis ? size.x : size.z;
		const int depth = std::min(p_stripDepth, std::max(1, extent / 2));
		for (const TraversalGraph::Node &node : p_board.navigation().graph().allNodes())
		{
			if (inPlayerStrip(node.position, size, depth, p_playerFacing))
			{
				result.player.push_back(node.position);
			}
			if (inEnemyStrip(node.position, size, depth, p_playerFacing))
			{
				result.enemy.push_back(node.position);
			}
		}
		return result;
	}
}
