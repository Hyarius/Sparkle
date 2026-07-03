#include "battle/battle_context.hpp"

#include "core/event_center.hpp"

#include <algorithm>

namespace pg
{
	BattleContext::BattleContext(EventCenter &p_events, BoardData p_board) :
		_events(p_events),
		board(std::move(p_board))
	{
	}

	BattleUnit &BattleContext::addUnit(CreatureUnit *p_source, BattleSide p_side)
	{
		auto unit = std::make_unique<BattleUnit>(p_source, p_side);
		BattleUnit &result = *unit;
		_storage.push_back(std::move(unit));
		(p_side == BattleSide::Player ? _playerUnits : _enemyUnits).push_back(&result);
		return result;
	}

	BattleUnit &BattleContext::addUnit(CreatureUnit &p_source, BattleSide p_side)
	{
		return addUnit(&p_source, p_side);
	}

	const std::vector<BattleUnit *> &BattleContext::getUnits(BattleSide p_side) const
	{
		static const std::vector<BattleUnit *> empty;
		return p_side == BattleSide::Player ? _playerUnits : (p_side == BattleSide::Enemy ? _enemyUnits : empty);
	}

	std::vector<BattleUnit *> BattleContext::getOpponents(const BattleUnit &p_unit) const
	{
		return p_unit.side == BattleSide::Player ? _enemyUnits : _playerUnits;
	}

	std::vector<BattleUnit *> BattleContext::allUnits() const
	{
		std::vector<BattleUnit *> result;
		result.reserve(_storage.size());
		for (const auto &unit : _storage)
		{
			result.push_back(unit.get());
		}
		return result;
	}

	bool BattleContext::hasLivingUnits(BattleSide p_side) const
	{
		return std::ranges::any_of(getUnits(p_side), [](const BattleUnit *p_unit) {
			return p_unit->isActiveInBattle();
		});
	}

	bool BattleContext::tryPlaceUnit(BattleUnit &p_unit, const spk::Vector3Int &p_cell)
	{
		if (!board.tryPlace(p_unit, p_cell))
		{
			return false;
		}
		p_unit.boardPosition = p_cell;
		_events.battleUnitPlaced.trigger(&p_unit);
		return true;
	}
	bool BattleContext::tryMoveUnit(BattleUnit &p_unit, const spk::Vector3Int &p_cell)
	{
		if (!board.tryMove(p_unit, p_cell))
		{
			return false;
		}
		p_unit.boardPosition = p_cell;
		return true;
	}
	bool BattleContext::trySwapUnits(BattleUnit &p_first, BattleUnit &p_second)
	{
		const auto first = p_first.boardPosition, second = p_second.boardPosition;
		if (!first || !second || !board.swapUnits(p_first, p_second))
		{
			return false;
		}
		p_first.boardPosition = second;
		p_second.boardPosition = first;
		return true;
	}
	bool BattleContext::removeUnit(BattleUnit &p_unit)
	{
		const bool removed = board.remove(p_unit);
		p_unit.boardPosition.reset();
		return removed;
	}
	bool BattleContext::defeatUnit(BattleUnit &p_unit)
	{
		return removeUnit(p_unit);
	}

	void BattleContext::report(BattleEvent p_event)
	{
		log.append(std::move(p_event));
		_events.battleEventOccurred.trigger(&log.events().back());
	}
	EventCenter &BattleContext::events() noexcept
	{
		return _events;
	}
}
