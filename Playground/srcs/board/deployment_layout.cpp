#include "board/deployment_layout.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace pg
{
	namespace
	{
		// The player deploys on the edge it walked in from: a +Z approach means it entered moving
		// toward +Z, so it stands on the low-Z edge and the enemy waits on the high-Z one. This is the
		// same mapping the step-04 board definition validates its strips with.
		[[nodiscard]] bool playerHoldsLowEdge(VoxelOrientation p_approach) noexcept
		{
			return p_approach == VoxelOrientation::PositiveZ || p_approach == VoxelOrientation::PositiveX;
		}
	}

	bool isHorizontalApproach(VoxelOrientation p_approach) noexcept
	{
		return p_approach == VoxelOrientation::PositiveX || p_approach == VoxelOrientation::NegativeX ||
			   p_approach == VoxelOrientation::PositiveZ || p_approach == VoxelOrientation::NegativeZ;
	}

	bool approachAxisIsZ(VoxelOrientation p_approach) noexcept
	{
		return p_approach == VoxelOrientation::PositiveZ || p_approach == VoxelOrientation::NegativeZ;
	}

	bool DeploymentStrips::isPlayerRow(int p_axisCoordinate) const noexcept
	{
		return p_axisCoordinate >= playerMinimum && p_axisCoordinate <= playerMaximum;
	}

	bool DeploymentStrips::isEnemyRow(int p_axisCoordinate) const noexcept
	{
		return p_axisCoordinate >= enemyMinimum && p_axisCoordinate <= enemyMaximum;
	}

	DeploymentStrips deploymentStrips(
		const spk::Vector2Int &p_boardSize,
		VoxelOrientation p_playerApproach,
		int p_stripDepth)
	{
		if (!isHorizontalApproach(p_playerApproach))
		{
			throw std::invalid_argument("a deployment approach is horizontal: a vertical one has no strip");
		}
		if (p_boardSize.x <= 0 || p_boardSize.y <= 0)
		{
			throw std::invalid_argument("a board extent is strictly positive on both horizontal axes");
		}

		DeploymentStrips strips;
		strips.axisIsZ = approachAxisIsZ(p_playerApproach);

		const int extent = strips.axisIsZ ? p_boardSize.y : p_boardSize.x;
		if (p_stripDepth <= 0 || 2 * p_stripDepth > extent)
		{
			throw std::invalid_argument(
				"deployment depth " + std::to_string(p_stripDepth) + " does not fit twice in the " +
				std::to_string(extent) + "-row approach axis");
		}

		const int lowMinimum = 0;
		const int lowMaximum = p_stripDepth - 1;
		const int highMinimum = extent - p_stripDepth;
		const int highMaximum = extent - 1;
		if (playerHoldsLowEdge(p_playerApproach))
		{
			strips.playerMinimum = lowMinimum;
			strips.playerMaximum = lowMaximum;
			strips.enemyMinimum = highMinimum;
			strips.enemyMaximum = highMaximum;
		}
		else
		{
			strips.playerMinimum = highMinimum;
			strips.playerMaximum = highMaximum;
			strips.enemyMinimum = lowMinimum;
			strips.enemyMaximum = lowMaximum;
		}
		return strips;
	}

	bool DeploymentLayout::isDeploymentAxisZ() const noexcept
	{
		return approachAxisIsZ(playerApproachDirection);
	}

	int DeploymentLayout::deploymentAxisCoordinate(const BoardCell &p_cell) const noexcept
	{
		return isDeploymentAxisZ() ? p_cell.z : p_cell.x;
	}

	int DeploymentLayout::perpendicularCoordinate(const BoardCell &p_cell) const noexcept
	{
		return isDeploymentAxisZ() ? p_cell.x : p_cell.z;
	}

	int DeploymentLayout::rowIndexFromOuterEdge(BattleSide p_side, const BoardCell &p_cell) const noexcept
	{
		const int extent = isDeploymentAxisZ() ? boardSize.y : boardSize.x;
		const int coordinate = deploymentAxisCoordinate(p_cell);
		const bool low = (p_side == BattleSide::Player) == playerHoldsLowEdge(playerApproachDirection);
		return low ? coordinate : extent - 1 - coordinate;
	}

	bool DeploymentLayout::contains(BattleSide p_side, BoardCell p_cell) const noexcept
	{
		const std::span<const BoardCell> zone = cells(p_side);
		return std::ranges::find(zone, p_cell) != zone.end();
	}

	std::span<const BoardCell> DeploymentLayout::cells(BattleSide p_side) const noexcept
	{
		return p_side == BattleSide::Player ? std::span<const BoardCell>(playerCells)
											: std::span<const BoardCell>(enemyCells);
	}

	DeploymentLayout buildDeploymentLayout(
		const TraversalGraph &p_navigation,
		const spk::Vector2Int &p_boardSize,
		VoxelOrientation p_playerApproach,
		int p_stripDepth)
	{
		const DeploymentStrips strips = deploymentStrips(p_boardSize, p_playerApproach, p_stripDepth);

		DeploymentLayout layout;
		layout.stripDepth = p_stripDepth;
		layout.playerApproachDirection = p_playerApproach;
		layout.boardSize = p_boardSize;

		// Every standable node whose column falls in a strip belongs to it - including two nodes of
		// one column, so a bridge over a walkable floor offers both of its levels.
		for (const TraversalGraph::Node &node : p_navigation.allNodes())
		{
			const int coordinate = layout.deploymentAxisCoordinate(node.position);
			if (strips.isPlayerRow(coordinate))
			{
				layout.playerCells.push_back(node.position);
			}
			else if (strips.isEnemyRow(coordinate))
			{
				layout.enemyCells.push_back(node.position);
			}
		}

		// The base order: rows from the board centre outward - so the first candidates are the ones
		// closest to the fight - then left to right across the row, then upward in a column holding
		// several standable levels, then the full cell order as a last resort.
		const auto centreFirst = [&layout](BattleSide p_side) {
			return [&layout, p_side](const BoardCell &p_left, const BoardCell &p_right) {
				const int leftRow = layout.rowIndexFromOuterEdge(p_side, p_left);
				const int rightRow = layout.rowIndexFromOuterEdge(p_side, p_right);
				if (leftRow != rightRow)
				{
					return leftRow > rightRow;
				}
				const int leftPerpendicular = layout.perpendicularCoordinate(p_left);
				const int rightPerpendicular = layout.perpendicularCoordinate(p_right);
				if (leftPerpendicular != rightPerpendicular)
				{
					return leftPerpendicular < rightPerpendicular;
				}
				if (p_left.y != p_right.y)
				{
					return p_left.y < p_right.y;
				}
				return BoardCellLess{}(p_left, p_right);
			};
		};
		std::ranges::sort(layout.playerCells, centreFirst(BattleSide::Player));
		std::ranges::sort(layout.enemyCells, centreFirst(BattleSide::Enemy));
		return layout;
	}
}
