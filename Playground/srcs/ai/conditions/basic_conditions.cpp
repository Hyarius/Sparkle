#include "ai/conditions/basic_conditions.hpp"

#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "creatures/creature_unit.hpp"

#include <algorithm>
#include <cmath>

namespace
{
	[[nodiscard]] int distance(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) noexcept
	{
		return std::abs(p_left.x - p_right.x) + std::abs(p_left.z - p_right.z);
	}

	[[nodiscard]] bool hpBelow(const pg::BattleUnit &p_unit, float p_percent) noexcept
	{
		return p_unit.attributes.hp.ratio() * 100.0f < p_percent;
	}
}

namespace pg
{
	bool EnemyWithinRangeCondition::isMet(const BattleUnit &p_unit, const BattleContext &p_context) const
	{
		if (!p_unit.boardPosition)
		{
			return false;
		}
		return std::ranges::any_of(p_context.getOpponents(p_unit), [&](const BattleUnit *p_enemy) {
			return p_enemy != nullptr && p_enemy->isActiveInBattle() && p_enemy->boardPosition &&
				distance(*p_unit.boardPosition, *p_enemy->boardPosition) <= range;
		});
	}

	bool AllyHpBelowCondition::isMet(const BattleUnit &p_unit, const BattleContext &p_context) const
	{
		return std::ranges::any_of(p_context.getUnits(p_unit.side), [&](const BattleUnit *p_ally) {
			return p_ally != nullptr && p_ally != &p_unit && p_ally->isActiveInBattle() && hpBelow(*p_ally, percent);
		});
	}

	bool SelfHpBelowCondition::isMet(const BattleUnit &p_unit, const BattleContext &) const
	{
		return p_unit.isActiveInBattle() && hpBelow(p_unit, percent);
	}

	bool HasStatusCondition::isMet(const BattleUnit &p_unit, const BattleContext &p_context) const
	{
		if (subject == AIStatusSubject::Self)
		{
			return p_unit.statuses.contains(statusId);
		}
		return std::ranges::any_of(p_context.getOpponents(p_unit), [&](const BattleUnit *p_enemy) {
			return p_enemy != nullptr && p_enemy->isActiveInBattle() && p_enemy->statuses.contains(statusId);
		});
	}

	bool CanCastCondition::isMet(const BattleUnit &p_unit, const BattleContext &p_context) const
	{
		return ability != nullptr && p_unit.source() != nullptr &&
			std::ranges::find(p_unit.source()->abilities, ability) != p_unit.source()->abilities.end() &&
			BattleActionValidator::canUseAbility(p_context, p_unit, *ability);
	}
}
