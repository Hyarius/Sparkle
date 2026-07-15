#pragma once

#include "battle/query/battle_plans.hpp"

#include <expected>
#include <string_view>
#include <vector>

namespace pg
{
	class BattleContext;
	class BattleQueryService
	{
	private:
		const BattleContext &_context;

	public:
		explicit BattleQueryService(const BattleContext &p_context) noexcept :
			_context(p_context)
		{
		}
		[[nodiscard]] std::expected<MovePlan, CommandRejection> planMove(BattleUnitId, BoardCell) const;
		[[nodiscard]] std::expected<CastPlan, CommandRejection> planCast(BattleUnitId, std::string_view, BoardCell) const;
		[[nodiscard]] std::vector<MovePlan> legalMoves(BattleUnitId) const;
		[[nodiscard]] std::vector<AbilityAnchorPreview> abilityAnchors(BattleUnitId, std::string_view) const;
		[[nodiscard]] bool hasAnyLegalNonEndCommand(BattleUnitId) const;
	};
}
