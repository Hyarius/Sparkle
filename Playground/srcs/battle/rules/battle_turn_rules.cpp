#include "battle/rules/battle_turn_rules.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "core/event_center.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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
				if (unit->isActiveInBattle() && unit->isTurnReady())
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
		p_context.report({.type = BattleEventType::TurnStarted, .turnIndex = p_context.currentTurn.turnIndex, .caster = &p_unit});
	}
	void BattleTurnRules::endTurn(BattleContext &p_context)
	{
		BattleUnit *unit = p_context.currentTurn.activeUnit;
		if (!unit)
		{
			return;
		}
		unit->attributes.advanceShieldDurations();
		(void)p_context.board.runtime().advanceObjectDurations();
		unit->attributes.ap.reset();
		unit->attributes.mp.reset();
		unit->attributes.turnBar.setCurrent(0.0f);
		p_context.report({.type = BattleEventType::TurnEnded, .turnIndex = p_context.currentTurn.turnIndex, .caster = unit});
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
		for (const Ability *ability : unit->source().abilities)
		{
			if (ability && unit->attributes.ap.current() >= ability->apCost && unit->attributes.mp.current() >= ability->mpCost && !BattleActionValidator::getValidTargets(p_context, *unit, *ability).empty())
			{
				return true;
			}
		}
		return false;
	}
}
