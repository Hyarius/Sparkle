#include "battle/battle_unit.hpp"

#include <algorithm>

namespace pg
{
	BattleUnit::BattleUnit(BattleUnitSource p_source, BattleSide p_side) :
		_source(std::move(p_source)),
		attributes(_source.attributes)
	{
		side = p_side;
	}
	const BattleUnitSource &BattleUnit::source() const noexcept
	{
		return _source;
	}
	const std::string &BattleUnit::displayName() const noexcept
	{
		return _source.displayName;
	}
	bool BattleUnit::isDefeated() const noexcept
	{
		return attributes.hp.current() <= 0;
	}
	bool BattleUnit::isActiveInBattle() const noexcept
	{
		return !isDefeated() && !hasLeftBattle;
	}
	bool BattleUnit::isTurnReady() const noexcept
	{
		return attributes.turnBar.current() >= attributes.turnBar.max();
	}
	bool BattleUnit::hasStatusTag(std::string_view p_tag) const
	{
		return std::ranges::find(statusTags, p_tag) != statusTags.end();
	}
}
