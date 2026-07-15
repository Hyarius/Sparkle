#pragma once

#include "battle/battle_command_result.hpp"
#include "battle/battle_ids.hpp"
#include "board/board_cell.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct PlannedPathStep
	{
		BoardCell from;
		BoardCell to;
		std::uint32_t movementPointCost = 0;
	};
	struct MovePlan
	{
		BattleUnitId unit;
		BoardCell origin;
		BoardCell destination;
		std::vector<PlannedPathStep> steps;
		std::uint32_t totalMovementPointCost = 0;
	};
	struct CastPlan
	{
		BattleUnitId source;
		std::string abilityId;
		BoardCell sourceCellAtCapture;
		BoardCell anchor;
		std::optional<BattleUnitId> primaryUnit;
		std::vector<BoardCell> areaCells;
		std::vector<BoardCell> affectedCells;
		std::vector<BattleUnitId> affectedUnits;
	};
	struct AbilityAnchorPreview
	{
		BoardCell anchor;
		bool legal = false;
		std::vector<CommandRejection> reasons;
		std::optional<CastPlan> plan;
	};
}
