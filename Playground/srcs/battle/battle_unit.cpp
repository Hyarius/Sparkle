#include "battle/battle_unit.hpp"

#include "creatures/creature_unit.hpp"

#include <algorithm>
#include <stdexcept>

namespace pg
{
	BattleUnit::BattleUnit(CreatureUnit *p_source, BattleSide p_side) :
		_source(p_source),
		attributes(p_source != nullptr ? p_source->attributes : Attributes{})
	{
		if (_source == nullptr)
		{
			throw std::invalid_argument("battle unit requires a creature source");
		}
		side = p_side;
	}

	BattleUnit::BattleUnit(CreatureUnit &p_source, BattleSide p_side) :
		BattleUnit(&p_source, p_side)
	{
	}

	BattleUnit::~BattleUnit() = default;

	CreatureUnit *BattleUnit::source() const noexcept
	{
		return _source;
	}

	const std::string &BattleUnit::displayName() const noexcept
	{
		return _source->displayName();
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
		return statuses.hasTag(p_tag) || std::ranges::find(statusTags, p_tag) != statusTags.end();
	}
}
