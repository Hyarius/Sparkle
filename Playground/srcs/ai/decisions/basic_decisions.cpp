#include "ai/decisions/basic_decisions.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "battle/rules/battle_geometry.hpp"
#include "board/line_of_sight.hpp"
#include "creatures/creature_unit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

namespace
{
	[[nodiscard]] int distance(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) noexcept
	{
		return std::abs(p_left.x - p_right.x) + std::abs(p_left.z - p_right.z);
	}

	[[nodiscard]] bool knowsAbility(const pg::BattleUnit &p_unit, const pg::Ability &p_ability)
	{
		const pg::CreatureUnit *source = p_unit.source();
		return source != nullptr && std::ranges::find(source->abilities, &p_ability) != source->abilities.end();
	}

	[[nodiscard]] std::vector<pg::BattleUnit *> targetUnits(
		pg::BattleUnit &p_unit, pg::BattleContext &p_context, pg::AITarget p_target)
	{
		if (p_target == pg::AITarget::Self)
		{
			return {&p_unit};
		}
		std::vector<pg::BattleUnit *> result = p_target == pg::AITarget::LowestHpAlly
			? p_context.getUnits(p_unit.side)
			: p_context.getOpponents(p_unit);
		std::erase_if(result, [&](const pg::BattleUnit *p_candidate) {
			return p_candidate == nullptr || !p_candidate->isActiveInBattle() || !p_candidate->boardPosition ||
				(p_target == pg::AITarget::LowestHpAlly && p_candidate == &p_unit);
		});
		return result;
	}

	[[nodiscard]] bool targetProfileAllows(
		const pg::BattleUnit &p_unit, const pg::BattleUnit &p_target, pg::TargetProfile p_profile) noexcept
	{
		switch (p_profile)
		{
		case pg::TargetProfile::Everything:
			return true;
		case pg::TargetProfile::Ally:
			return p_target.side == p_unit.side;
		case pg::TargetProfile::Enemy:
			return p_target.side != p_unit.side;
		case pg::TargetProfile::Empty:
			return false;
		}
		return false;
	}

	[[nodiscard]] bool canTargetFrom(
		const pg::BattleUnit &p_unit,
		const pg::BattleUnit &p_target,
		const pg::BattleContext &p_context,
		const pg::Ability &p_ability,
		const spk::Vector3Int &p_from)
	{
		if (!p_target.boardPosition || !targetProfileAllows(p_unit, p_target, p_ability.targetProfile))
		{
			return false;
		}
		const spk::Vector2Int offset{
			p_target.boardPosition->x - p_from.x,
			p_target.boardPosition->z - p_from.z};
		const int maximumRange = p_ability.maximumRange + static_cast<int>(p_unit.attributes.bonusRange);
		return pg::BattleGeometry::rangeContains(
				   p_ability.rangeShape, offset, p_ability.minimumRange, maximumRange) &&
			(!p_ability.requiresLineOfSight ||
			 pg::LineOfSight::hasLine(p_context.board.terrain().cells(), p_from, *p_target.boardPosition));
	}

	[[nodiscard]] pg::BattleUnit *selectTarget(
		const std::vector<pg::BattleUnit *> &p_candidates,
		pg::AITarget p_target,
		const spk::Vector3Int &p_from)
	{
		if (p_candidates.empty())
		{
			return nullptr;
		}
		return *std::ranges::min_element(p_candidates, {}, [&](const pg::BattleUnit *p_candidate) {
			if (p_target == pg::AITarget::LowestHpEnemy || p_target == pg::AITarget::LowestHpAlly)
			{
				return std::tuple{p_candidate->attributes.hp.ratio(), distance(p_from, *p_candidate->boardPosition)};
			}
			return std::tuple{static_cast<float>(distance(p_from, *p_candidate->boardPosition)), 0};
		});
	}

	[[nodiscard]] int nearestEnemyDistance(
		const pg::BattleUnit &p_unit, const pg::BattleContext &p_context, const spk::Vector3Int &p_cell)
	{
		int result = std::numeric_limits<int>::max();
		for (const pg::BattleUnit *enemy : p_context.getOpponents(p_unit))
		{
			if (enemy != nullptr && enemy->isActiveInBattle() && enemy->boardPosition)
			{
				result = std::min(result, distance(p_cell, *enemy->boardPosition));
			}
		}
		return result;
	}
}

namespace pg
{
	std::unique_ptr<BattleAction> CastAbilityDecision::produce(
		BattleUnit &p_unit, BattleContext &p_context) const
	{
		if (ability == nullptr || !p_unit.boardPosition || !knowsAbility(p_unit, *ability) ||
			p_unit.attributes.ap.current() < ability->apCost || p_unit.attributes.mp.current() < ability->mpCost)
		{
			return nullptr;
		}

		std::vector<BattleUnit *> candidates = targetUnits(p_unit, p_context, target);
		std::vector<BattleUnit *> castable;
		for (BattleUnit *candidate : candidates)
		{
			if (canTargetFrom(p_unit, *candidate, p_context, *ability, *p_unit.boardPosition))
			{
				castable.push_back(candidate);
			}
		}
		if (BattleUnit *selected = selectTarget(castable, target, *p_unit.boardPosition); selected != nullptr)
		{
			return std::make_unique<AbilityAction>(
				p_unit, *ability, std::vector<spk::Vector3Int>{*selected->boardPosition});
		}

		if (target == AITarget::Self)
		{
			return nullptr;
		}
		const int movementBudget = p_unit.attributes.mp.current() - ability->mpCost;
		if (movementBudget <= 0)
		{
			return nullptr;
		}
		const auto reachable = BattleActionValidator::getReachableCells(p_context, p_unit);
		std::optional<spk::Vector3Int> bestCell;
		int bestCost = std::numeric_limits<int>::max();
		for (const auto &[cell, rawCost] : reachable)
		{
			const int cost = static_cast<int>(rawCost);
			if (cost <= 0 || cost > movementBudget)
			{
				continue;
			}
			const bool canCastThere = std::ranges::any_of(candidates, [&](const BattleUnit *p_candidate) {
				return canTargetFrom(p_unit, *p_candidate, p_context, *ability, cell);
			});
			if (canCastThere && (!bestCell || cost < bestCost || (cost == bestCost && CellPositionLess{}(cell, *bestCell))))
			{
				bestCell = cell;
				bestCost = cost;
			}
		}
		return bestCell ? std::make_unique<MoveAction>(p_unit, *bestCell, bestCost) : nullptr;
	}

	std::unique_ptr<BattleAction> MoveTowardNearestEnemyDecision::produce(
		BattleUnit &p_unit, BattleContext &p_context) const
	{
		if (!p_unit.boardPosition)
		{
			return nullptr;
		}
		const int currentDistance = nearestEnemyDistance(p_unit, p_context, *p_unit.boardPosition);
		std::optional<spk::Vector3Int> bestCell;
		int bestDistance = currentDistance;
		int bestCost = 0;
		for (const auto &[cell, rawCost] : BattleActionValidator::getReachableCells(p_context, p_unit))
		{
			const int cost = static_cast<int>(rawCost);
			const int candidateDistance = nearestEnemyDistance(p_unit, p_context, cell);
			if (cost > 0 && (candidateDistance < bestDistance ||
				(candidateDistance == bestDistance && cost > bestCost)))
			{
				bestCell = cell;
				bestDistance = candidateDistance;
				bestCost = cost;
			}
		}
		return bestCell ? std::make_unique<MoveAction>(p_unit, *bestCell, bestCost) : nullptr;
	}

	std::unique_ptr<BattleAction> MoveAwayFromEnemiesDecision::produce(
		BattleUnit &p_unit, BattleContext &p_context) const
	{
		if (!p_unit.boardPosition)
		{
			return nullptr;
		}
		const int currentDistance = nearestEnemyDistance(p_unit, p_context, *p_unit.boardPosition);
		std::optional<spk::Vector3Int> bestCell;
		int bestDistance = currentDistance;
		int bestCost = 0;
		for (const auto &[cell, rawCost] : BattleActionValidator::getReachableCells(p_context, p_unit))
		{
			const int cost = static_cast<int>(rawCost);
			const int candidateDistance = nearestEnemyDistance(p_unit, p_context, cell);
			if (cost > 0 && (candidateDistance > bestDistance ||
				(candidateDistance == bestDistance && cost < bestCost)))
			{
				bestCell = cell;
				bestDistance = candidateDistance;
				bestCost = cost;
			}
		}
		return bestCell ? std::make_unique<MoveAction>(p_unit, *bestCell, bestCost) : nullptr;
	}

	std::unique_ptr<BattleAction> EndTurnDecision::produce(BattleUnit &p_unit, BattleContext &) const
	{
		return std::make_unique<EndTurnAction>(p_unit);
	}
}
