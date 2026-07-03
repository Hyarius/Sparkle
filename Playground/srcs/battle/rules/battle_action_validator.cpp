#include "battle/rules/battle_action_validator.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_geometry.hpp"
#include "board/line_of_sight.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace pg
{
	bool BattleActionValidator::canAfford(const BattleUnit &p_unit, const BattleAction &p_action)
	{
		return p_unit.attributes.ap.current() >= p_action.apCost() && p_unit.attributes.mp.current() >= p_action.mpCost();
	}

	bool BattleActionValidator::canUseAbility(
		const BattleContext &p_context, const BattleUnit &p_unit, const Ability &p_ability)
	{
		return p_unit.isActiveInBattle() && p_unit.attributes.ap.current() >= p_ability.apCost &&
			   p_unit.attributes.mp.current() >= p_ability.mpCost && !getValidTargets(p_context, p_unit, p_ability).empty();
	}

	std::map<spk::Vector3Int, float, CellPositionLess> BattleActionValidator::getReachableCells(
		const BattleContext &p_context, const BattleUnit &p_unit)
	{
		std::map<spk::Vector3Int, float, CellPositionLess> result;
		if (!p_unit.boardPosition)
		{
			return result;
		}
		const TraversalGraph &graph = p_context.board.navigation().graph();
		const auto start = graph.indexOf(*p_unit.boardPosition);
		if (!start)
		{
			return result;
		}
		std::queue<std::size_t> open;
		std::vector<int> distance(graph.size(), -1);
		distance[*start] = 0;
		open.push(*start);
		while (!open.empty())
		{
			const auto current = open.front();
			open.pop();
			const auto &node = graph.node(current);
			result[node.position] = static_cast<float>(distance[current]);
			for (auto neighbor : node.neighbors)
			{
				if (!neighbor || distance[*neighbor] >= 0)
				{
					continue;
				}
				const auto pos = graph.node(*neighbor).position;
				if (p_context.board.runtime().hasUnitAt(pos))
				{
					continue;
				}
				const int next = distance[current] + 1;
				if (next > p_unit.attributes.mp.current())
				{
					continue;
				}
				distance[*neighbor] = next;
				open.push(*neighbor);
			}
		}
		return result;
	}

	std::vector<spk::Vector3Int> BattleActionValidator::getValidTargets(
		const BattleContext &p_context, const BattleUnit &p_unit, const Ability &p_ability)
	{
		std::vector<spk::Vector3Int> result;
		if (!p_unit.boardPosition)
		{
			return result;
		}
		const int maximumRange = p_ability.maximumRange + static_cast<int>(p_unit.attributes.bonusRange);
		for (const auto &node : p_context.board.navigation().graph().allNodes())
		{
			const spk::Vector2Int offset{
				node.position.x - p_unit.boardPosition->x,
				node.position.z - p_unit.boardPosition->z};
			if (!BattleGeometry::rangeContains(p_ability.rangeShape, offset, p_ability.minimumRange, maximumRange))
			{
				continue;
			}
			BattleObject *at = p_context.tryGetUnitAtIncludingDefeated(node.position);
			bool profile = false;
			switch (p_ability.targetProfile)
			{
			case TargetProfile::Everything:
				profile = true;
				break;
			case TargetProfile::Empty:
				profile = at == nullptr;
				break;
			case TargetProfile::Ally:
				profile = at != nullptr && at->side == p_unit.side;
				break;
			case TargetProfile::Enemy:
				profile = at != nullptr && at->side != p_unit.side;
				break;
			}
			if (!profile)
			{
				continue;
			}
			if (p_ability.requiresLineOfSight && !LineOfSight::hasLine(p_context.board.terrain().cells(), *p_unit.boardPosition, node.position))
			{
				continue;
			}
			result.push_back(node.position);
		}
		return result;
	}

	RangeShading BattleActionValidator::getRangeShading(
		const BattleContext &p_context, const BattleUnit &p_unit, const Ability &p_ability)
	{
		RangeShading result;
		if (!p_unit.boardPosition)
		{
			return result;
		}
		const int maximumRange = p_ability.maximumRange + static_cast<int>(p_unit.attributes.bonusRange);
		for (const auto &node : p_context.board.navigation().graph().allNodes())
		{
			const spk::Vector2Int offset{
				node.position.x - p_unit.boardPosition->x,
				node.position.z - p_unit.boardPosition->z};
			if (!BattleGeometry::rangeContains(p_ability.rangeShape, offset, p_ability.minimumRange, maximumRange))
			{
				continue;
			}
			const bool blocked = p_ability.requiresLineOfSight &&
								 !LineOfSight::hasLine(p_context.board.terrain().cells(), *p_unit.boardPosition, node.position);
			(blocked ? result.blocked : result.visible).push_back(node.position);
		}
		return result;
	}

	std::vector<spk::Vector3Int> BattleActionValidator::getAreaCells(
		const BattleContext &p_context,
		const BattleUnit &p_unit,
		const Ability &p_ability,
		const spk::Vector3Int &p_anchor)
	{
		std::vector<spk::Vector3Int> result;
		const spk::Vector2Int casterToAnchor = p_unit.boardPosition
												   ? spk::Vector2Int{p_anchor.x - p_unit.boardPosition->x, p_anchor.z - p_unit.boardPosition->z}
												   : spk::Vector2Int{0, 0};
		const std::vector<spk::Vector2Int> offsets =
			BattleGeometry::areaOffsets(p_ability.areaShape, p_ability.areaValue, casterToAnchor);
		const auto &nodes = p_context.board.navigation().graph().allNodes();
		for (const spk::Vector2Int &offset : offsets)
		{
			const auto node = std::ranges::find_if(nodes, [&](const auto &candidate) {
				return candidate.position.x == p_anchor.x + offset.x && candidate.position.z == p_anchor.z + offset.y;
			});
			if (node != nodes.end())
			{
				result.push_back(node->position);
			}
		}
		return result;
	}

	bool BattleActionValidator::validate(const BattleContext &p_context, const BattleAction &p_action)
	{
		if (!p_action.source.isActiveInBattle() || !canAfford(p_action.source, p_action))
		{
			return false;
		}
		if (p_action.kind() == BattleActionKind::EndTurn)
		{
			return true;
		}
		if (p_action.kind() == BattleActionKind::Move)
		{
			const auto &move = static_cast<const MoveAction &>(p_action);
			const auto reachable = getReachableCells(p_context, p_action.source);
			const auto it = reachable.find(move.destination);
			return it != reachable.end() && static_cast<int>(it->second) == move.distance;
		}
		const auto &ability = static_cast<const AbilityAction &>(p_action);
		if (ability.targetCells.empty())
		{
			return false;
		}
		const auto valid = getValidTargets(p_context, p_action.source, ability.ability);
		return std::ranges::all_of(ability.targetCells, [&](const spk::Vector3Int &cell) {
			return std::ranges::find(valid, cell) != valid.end();
		});
	}
}
