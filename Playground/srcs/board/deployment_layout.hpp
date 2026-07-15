#pragma once

#include "battle/battle_types.hpp"
#include "board/board_cell.hpp"
#include "board/traversal_graph.hpp"
#include "voxel/voxel_enums.hpp"

#include "structures/math/spk_vector2.hpp"

#include <span>
#include <vector>

namespace pg
{
	[[nodiscard]] bool isHorizontalApproach(VoxelOrientation p_approach) noexcept;
	// The strips are cut across the approach axis: Z for a +Z/-Z approach, X otherwise. Callers that
	// need the approach-axis extent of a board ask this before they have a layout.
	[[nodiscard]] bool approachAxisIsZ(VoxelOrientation p_approach) noexcept;

	// The two strips as inclusive coordinate ranges along the deployment axis, without a graph. The
	// live board builder scans candidate anchors for standable ground through this before any graph
	// exists, and buildDeploymentLayout fills the same ranges afterwards, so a candidate can never be
	// accepted against one mapping and deployed against another.
	struct DeploymentStrips
	{
		bool axisIsZ = true;
		int playerMinimum = 0;
		int playerMaximum = 0;
		int enemyMinimum = 0;
		int enemyMaximum = 0;

		[[nodiscard]] bool isPlayerRow(int p_axisCoordinate) const noexcept;
		[[nodiscard]] bool isEnemyRow(int p_axisCoordinate) const noexcept;
	};

	// Throws std::invalid_argument on a vertical approach, a non-positive extent or depth, or a depth
	// whose two strips would overlap (2 * depth must fit inside the approach-axis extent).
	[[nodiscard]] DeploymentStrips deploymentStrips(
		const spk::Vector2Int &p_boardSize,
		VoxelOrientation p_playerApproach,
		int p_stripDepth);

	// The two strips a team may be placed in, as data. A deployment cell is not occupied until a
	// placement command succeeds; this only says where placement is legal and in what canonical
	// order candidates are considered.
	//
	// The approach is the player's last move into the encounter, so the player deploys on the edge
	// it walked in from and the enemy waits on the far one:
	//   +Z -> player low-Z strip, enemy high-Z strip      -Z -> the mirror
	//   +X -> player low-X strip, enemy high-X strip      -X -> the mirror
	//
	// Cells are stored centre-first: the row nearest the board centre, then rows out toward the
	// outer edge; inside a row, increasing perpendicular coordinate, then Y, then BoardCellLess.
	// Manual placement and seeded-random candidate order read that base order directly. Authored
	// byLine placement does not: it re-sorts from rowIndexFromOuterEdge (which is exactly step-04's
	// rowsFromEnemyEdge for the enemy side) plus its own left/right/centre-out policy, which is why
	// the row/perpendicular helpers below exist instead of that algorithm having to recover the
	// orientation from an unordered pile of cells.
	struct DeploymentLayout
	{
		int stripDepth = 0;
		VoxelOrientation playerApproachDirection = VoxelOrientation::PositiveZ;
		// x is the board's X extent, y its Z extent (Vector2Int has no z). Kept here so a row index
		// can be measured from the far edge without the caller passing the board back in.
		spk::Vector2Int boardSize{};
		std::vector<BoardCell> playerCells;
		std::vector<BoardCell> enemyCells;

		[[nodiscard]] bool contains(BattleSide p_side, BoardCell p_cell) const noexcept;
		[[nodiscard]] std::span<const BoardCell> cells(BattleSide p_side) const noexcept;

		// The strips are cut across the approach axis: Z for a +Z/-Z approach, X otherwise.
		[[nodiscard]] bool isDeploymentAxisZ() const noexcept;
		// The cell's coordinate along the deployment axis, and across it. "Left" is the smaller
		// perpendicular coordinate.
		[[nodiscard]] int deploymentAxisCoordinate(const BoardCell &p_cell) const noexcept;
		[[nodiscard]] int perpendicularCoordinate(const BoardCell &p_cell) const noexcept;
		// 0 is that side's own outer edge row, growing toward the board centre. For the enemy side
		// this is step-04's authored rowsFromEnemyEdge.
		[[nodiscard]] int rowIndexFromOuterEdge(BattleSide p_side, const BoardCell &p_cell) const noexcept;
	};

	// Collects the standable nodes of both strips out of the frozen navigation graph. Every
	// standable node whose column lies in a strip belongs to it, including several vertical nodes in
	// one column (a bridge over a walkable floor deploys on either).
	//
	// Throws std::invalid_argument on a vertical approach, a non-positive depth, or a depth whose
	// two strips would overlap (2 * depth must fit inside the approach-axis extent): a board whose
	// teams would start inside each other is a broken board, not a placement to be repaired.
	[[nodiscard]] DeploymentLayout buildDeploymentLayout(
		const TraversalGraph &p_navigation,
		const spk::Vector2Int &p_boardSize,
		VoxelOrientation p_playerApproach,
		int p_stripDepth);
}
