#pragma once

#include "battle/battle_command_result.hpp" // BattleConstructionError
#include "battle/battle_rng.hpp"
#include "battle/battle_types.hpp"
#include "board/board_cell.hpp"
#include "board/board_data.hpp"
#include "encounters/encounter_definition.hpp" // OpponentPlacementPolicy

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	// The planned enemy destinations, in enemy roster order, or the transactional failure that
	// leaves the not-yet-published session discarded. Random draws happen only after every
	// non-random validation and capacity check has passed, so a failed plan consumes no RNG.
	struct EnemyPlacementPlan
	{
		std::vector<BoardCell> cells;
		std::optional<BattleConstructionError> error;

		[[nodiscard]] bool ok() const noexcept { return !error.has_value(); }
	};

	// A deployment destination is legal for a side when it is one of that side's standable strip
	// cells; the layout collected exactly those from the frozen graph.
	[[nodiscard]] bool isLegalDeploymentCell(const BoardData &p_board, BattleSide p_side, const BoardCell &p_cell) noexcept;

	// Plans every enemy destination before any mutation, following the exact step-04 policy variant:
	//   fixed         each authored [x, z] -> first standable enemy-zone support by increasing y then
	//                 BoardCellLess; a missing column, a duplicate cell or an insufficient list fails.
	//   byLine        rowsFromEnemyEdge validated against the real depth; rows enumerated from that
	//                 outer-edge offset toward centre without wrap, ordered within a row by the
	//                 authored leftToRight / rightToLeft / centerOut, then increasing y / BoardCellLess.
	//   seededRandom  canonical free enemy-zone cells, capacity proven first, then a partial
	//                 Fisher-Yates selection with BattleRng::uniformBelow per roster entry.
	[[nodiscard]] EnemyPlacementPlan planEnemyPlacements(
		const BoardData &p_board,
		const OpponentPlacementPolicy &p_policy,
		std::size_t p_enemyCount,
		BattleRng &p_rng,
		const std::string &p_encounterDefinitionId);
}
