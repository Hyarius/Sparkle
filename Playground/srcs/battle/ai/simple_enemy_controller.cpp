#include "battle/ai/simple_enemy_controller.hpp"

#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "board/pathfinder.hpp"

#include <algorithm>
#include <limits>

namespace pg
{
	std::unique_ptr<BattleAction> SimpleEnemyController::choose(BattleContext &p_context, BattleUnit &p_unit)
	{
		if (!p_unit.boardPosition)
		{
			return std::make_unique<EndTurnAction>(p_unit);
		}
		for (const Ability *ability : p_unit.source().abilities)
		{
			if (!ability)
			{
				continue;
			}
			auto targets = BattleActionValidator::getValidTargets(p_context, p_unit, *ability);
			if (!targets.empty())
			{
				return std::make_unique<AbilityAction>(p_unit, *ability, std::vector<spk::Vector3Int>{targets.front()});
			}
		}
		BattleUnit *nearest = nullptr;
		int best = std::numeric_limits<int>::max();
		for (BattleUnit *enemy : p_context.getOpponents(p_unit))
		{
			if (!enemy->isActiveInBattle() || !enemy->boardPosition)
			{
				continue;
			}
			const int d = std::abs(enemy->boardPosition->x - p_unit.boardPosition->x) + std::abs(enemy->boardPosition->z - p_unit.boardPosition->z);
			if (d < best)
			{
				best = d;
				nearest = enemy;
			}
		}
		if (!nearest)
		{
			return std::make_unique<EndTurnAction>(p_unit);
		}
		auto path = Pathfinder::findPath(p_context.board.navigation().graph(), *p_unit.boardPosition, *nearest->boardPosition);
		if (!path || path->size() < 3)
		{
			return std::make_unique<EndTurnAction>(p_unit);
		}
		const int steps = std::min<int>(p_unit.attributes.mp.current(), static_cast<int>(path->size()) - 2);
		if (steps <= 0)
		{
			return std::make_unique<EndTurnAction>(p_unit);
		}
		return std::make_unique<MoveAction>(p_unit, (*path)[steps], steps);
	}
}
