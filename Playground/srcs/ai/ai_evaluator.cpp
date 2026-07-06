#include "ai/ai_evaluator.hpp"

#include "ai/ai_behaviour.hpp"
#include "battle/battle_action.hpp"
#include "battle/rules/battle_action_validator.hpp"

#include <algorithm>

namespace pg
{
	std::unique_ptr<BattleAction> AIEvaluator::evaluate(
		const AIBehaviour &p_behaviour, BattleUnit &p_unit, BattleContext &p_context)
	{
		for (const AIRule &rule : p_behaviour.activeRules())
		{
			const bool conditionsMet = std::ranges::all_of(rule.conditions, [&](const auto &p_condition) {
				return p_condition != nullptr && p_condition->isMet(p_unit, p_context);
			});
			if (!conditionsMet || rule.decision == nullptr)
			{
				continue;
			}
			std::unique_ptr<BattleAction> action = rule.decision->produce(p_unit, p_context);
			if (action != nullptr && BattleActionValidator::validate(p_context, *action))
			{
				return action;
			}
		}
		return std::make_unique<EndTurnAction>(p_unit);
	}
}
