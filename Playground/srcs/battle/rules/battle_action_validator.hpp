#pragma once

#include "battle/battle_action.hpp"
#include "board/traversal_graph.hpp"

#include <map>
#include <vector>

namespace pg
{
	class BattleContext;

	// The board cells inside an ability's range band, split by line of sight: cells the caster
	// can see (draped with the range mask) versus cells blocked by terrain (draped with the
	// losBlocked mask instead — battle overlay work item 2). Neither set is target-profile
	// filtered; this powers the shading, while getValidTargets returns the legal anchors.
	struct RangeShading
	{
		std::vector<spk::Vector3Int> visible;
		std::vector<spk::Vector3Int> blocked;
	};

	class BattleActionValidator
	{
	public:
		[[nodiscard]] static bool canAfford(const BattleUnit &p_unit, const BattleAction &p_action);
		[[nodiscard]] static std::map<spk::Vector3Int, float, CellPositionLess> getReachableCells(
			const BattleContext &p_context, const BattleUnit &p_unit);
		[[nodiscard]] static std::vector<spk::Vector3Int> getValidTargets(
			const BattleContext &p_context, const BattleUnit &p_unit, const Ability &p_ability);
		[[nodiscard]] static RangeShading getRangeShading(
			const BattleContext &p_context, const BattleUnit &p_unit, const Ability &p_ability);
		[[nodiscard]] static std::vector<spk::Vector3Int> getAreaCells(
			const BattleContext &p_context,
			const BattleUnit &p_unit,
			const Ability &p_ability,
			const spk::Vector3Int &p_anchor);
		[[nodiscard]] static bool validate(const BattleContext &p_context, const BattleAction &p_action);
	};
}
