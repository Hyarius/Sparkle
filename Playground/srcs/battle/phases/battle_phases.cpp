#include "battle/phases/battle_phases.hpp"

#include "battle/ai/simple_enemy_controller.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_resolver.hpp"
#include "battle/rules/battle_outcome_rules.hpp"
#include "battle/rules/battle_placement_rules.hpp"
#include "battle/rules/battle_turn_rules.hpp"

namespace pg
{
	void SetupPhase::enter()
	{
		_done();
	}
	void PlacementPhase::enter()
	{
		_done(BattlePlacementRules::autoPlace(_context, _seed));
	}
	void IdlePhase::enter()
	{
		if (BattleOutcomeRules::winner(_context).has_value())
		{
			_ready(nullptr);
			return;
		}
		BattleUnit *unit = nullptr;
		for (std::size_t guard = 0; guard <= _context.allUnits().size(); ++guard)
		{
			BattleTurnRules::advanceToNextReady(_context);
			unit = BattleTurnRules::selectReady(_context);
			if (unit == nullptr)
			{
				break;
			}
			BattleTurnRules::beginTurn(_context, *unit);
			if (unit->isActiveInBattle())
			{
				break;
			}
			unit = nullptr;
			if (BattleOutcomeRules::winner(_context).has_value())
			{
				break;
			}
		}
		_ready(unit);
	}
	bool PlayerTurnPhase::submitAction(std::unique_ptr<BattleAction> p_action)
	{
		if (!_active || !p_action)
		{
			return false;
		}
		_chosen(std::move(p_action));
		return true;
	}
	void EnemyTurnPhase::enter()
	{
		BattleUnit *unit = _context.currentTurn.activeUnit;
		_chosen(unit ? SimpleEnemyController::choose(_context, *unit) : nullptr);
	}
	void ResolutionPhase::enter()
	{
		if (!_pending)
		{
			_done(BattleActionKind::EndTurn, false);
			return;
		}
		const BattleActionKind kind = _pending->kind();
		const bool result = BattleActionResolver::resolve(_context, *_pending);
		_done(kind, result);
	}
}
