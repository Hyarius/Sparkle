#include "battle/rules/battle_unit_rules.hpp"

#include "battle/battle_context.hpp"

namespace pg
{
	void BattleUnitRules::resolvePendingDefeats(
		BattleContext &p_context, BattleUnit *p_caster, const Ability *p_ability)
	{
		for (BattleUnit *unit : p_context.allUnits())
		{
			if (!unit->isDefeated() || !unit->boardPosition)
			{
				continue;
			}
			(void)p_context.defeatUnit(*unit);
			p_context.report({.type = BattleEventType::UnitDefeated, .turnIndex = p_context.currentTurn.turnIndex, .sourceAbility = p_ability, .caster = p_caster, .target = unit});
		}
	}
}
