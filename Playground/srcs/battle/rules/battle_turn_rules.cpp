#include "battle/rules/battle_turn_rules.hpp"
#include "creatures/creature_unit.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "battle/rules/battle_resource_rules.hpp"
#include "battle/rules/battle_status_rules.hpp"
#include "battle/rules/battle_unit_rules.hpp"
#include "core/event_center.hpp"
#include "statuses/status.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	[[nodiscard]] int nearestOpponentDistance(const pg::BattleContext &p_context, const pg::BattleUnit &p_unit)
	{
		if (!p_unit.boardPosition.has_value())
		{
			return std::numeric_limits<int>::max();
		}
		int result = std::numeric_limits<int>::max();
		for (const pg::BattleUnit *opponent : p_context.getOpponents(p_unit))
		{
			if (opponent == nullptr || !opponent->isActiveInBattle() || !opponent->boardPosition.has_value())
			{
				continue;
			}
			result = std::min(
				result,
				std::abs(p_unit.boardPosition->x - opponent->boardPosition->x) +
					std::abs(p_unit.boardPosition->z - opponent->boardPosition->z));
		}
		return result;
	}
}

namespace pg
{
	void BattleTurnRules::advanceTurnBars(BattleContext &p_context, float p_seconds)
	{
		if (p_seconds <= 0)
		{
			return;
		}
		for (BattleUnit *unit : p_context.allUnits())
		{
			if (!unit->isActiveInBattle() || unit->hasStatusTag("stun") || unit->attributes.staminaRate <= 0.0f)
			{
				continue;
			}
			unit->attributes.turnBar.setCurrent(std::min(unit->attributes.turnBar.max(), unit->attributes.turnBar.current() + p_seconds * static_cast<float>(unit->attributes.staminaRate)));
		}
	}
	float BattleTurnRules::timeUntilNextReady(const BattleContext &p_context)
	{
		float result = std::numeric_limits<float>::infinity();
		for (BattleUnit *unit : p_context.allUnits())
		{
			if (!unit->isActiveInBattle() || unit->hasStatusTag("stun") || unit->attributes.staminaRate <= 0.0f)
			{
				continue;
			}
			result = std::min(result, std::max(0.0f, (unit->attributes.turnBar.max() - unit->attributes.turnBar.current()) / static_cast<float>(unit->attributes.staminaRate)));
		}
		return result;
	}
	void BattleTurnRules::advanceToNextReady(BattleContext &p_context)
	{
		const float time = timeUntilNextReady(p_context);
		if (std::isfinite(time))
		{
			advanceTurnBars(p_context, time);
		}
	}
	BattleUnit *BattleTurnRules::selectReady(BattleContext &p_context)
	{
		for (BattleSide side : {BattleSide::Player, BattleSide::Enemy})
		{
			for (BattleUnit *unit : p_context.getUnits(side))
			{
				if (unit->isActiveInBattle() && !unit->hasStatusTag("stun") && unit->isTurnReady())
				{
					return unit;
				}
			}
		}
		return nullptr;
	}
	void BattleTurnRules::beginTurn(BattleContext &p_context, BattleUnit &p_unit)
	{
		p_context.currentTurn = {.activeUnit = &p_unit, .turnIndex = p_context.currentTurn.turnIndex + 1};
		p_unit.attributes.turnBar.setCurrent(p_unit.attributes.turnBar.max());
		p_context.report(TurnStartedEvent{
			.context = {.turnIndex = p_context.currentTurn.turnIndex, .caster = &p_unit},
			.nearestUnitDistance = nearestOpponentDistance(p_context, p_unit)});
		BattleStatusRules::applyHook(p_unit, StatusHookPoint::TurnStart);
		BattleUnitRules::resolvePendingDefeats(p_context, &p_unit);
	}
	void BattleTurnRules::endTurn(BattleContext &p_context)
	{
		BattleUnit *unit = p_context.currentTurn.activeUnit;
		if (!unit)
		{
			return;
		}
		BattleStatusRules::applyHook(*unit, StatusHookPoint::TurnEnd);
		BattleUnitRules::resolvePendingDefeats(p_context, unit);
		auto reportRemovals = [&](BattleUnit &owner, const std::vector<BattleStatusRemoval> &removed) {
			for (const BattleStatusRemoval &status : removed)
			{
				p_context.report(StatusRemovedEvent{.context = {.turnIndex = p_context.currentTurn.turnIndex, .caster = &owner, .target = &owner}, .status = status.definition, .count = status.stacks});
			}
		};
		reportRemovals(*unit, unit->statuses.advanceTurnDurations());
		for (BattleUnit *other : p_context.allUnits())
		{
			if (other != unit)
			{
				reportRemovals(*other, other->statuses.advanceTurnDurations(true, true));
			}
		}
		unit->attributes.advanceShieldDurations();
		(void)p_context.board.runtime().advanceObjectDurations();
		(void)BattleResourceRules::change(*unit, BattleResourceKind::AP, unit->attributes.ap.max() - unit->attributes.ap.current());
		(void)BattleResourceRules::change(*unit, BattleResourceKind::MP, unit->attributes.mp.max() - unit->attributes.mp.current());
		unit->attributes.turnBar.setCurrent(0.0f);
		p_context.report(TurnEndedEvent{
			.context = {.turnIndex = p_context.currentTurn.turnIndex, .caster = unit},
			.nearestUnitDistance = nearestOpponentDistance(p_context, *unit)});
		p_context.events().battleTurnEnded.trigger(unit);
		p_context.currentTurn.activeUnit = nullptr;
	}
	bool BattleTurnRules::canContinueTurn(const BattleContext &p_context)
	{
		BattleUnit *unit = p_context.currentTurn.activeUnit;
		if (!unit || !unit->isActiveInBattle())
		{
			return false;
		}
		const auto reachable = BattleActionValidator::getReachableCells(p_context, *unit);
		if (reachable.size() > 1)
		{
			return true;
		}
		for (const Ability *ability : unit->source()->abilities)
		{
			if (ability && unit->attributes.ap.current() >= ability->apCost && unit->attributes.mp.current() >= ability->mpCost && !BattleActionValidator::getValidTargets(p_context, *unit, *ability).empty())
			{
				return true;
			}
		}
		return false;
	}
}
