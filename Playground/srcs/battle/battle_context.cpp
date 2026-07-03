#include "battle/battle_context.hpp"

#include "core/event_center.hpp"
#include "creatures/creature_unit.hpp"
#include "statuses/status.hpp"

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
		result.statuses.bind(result, *this);
		if (p_source != nullptr)
		{
			for (const Status *passive : p_source->permanentPassives)
			{
				if (passive != nullptr)
				{
					(void)result.statuses.add(*passive, 1, {}, true);
				}
			}
		}
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
		p_unit.lastBoardPosition = p_cell;
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
		p_unit.lastBoardPosition = p_cell;
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
		p_first.lastBoardPosition = second;
		p_second.lastBoardPosition = first;
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

	BattleUnit *BattleContext::tryGetUnitAtIncludingDefeated(const spk::Vector3Int &p_cell) const
	{
		if (auto *unit = dynamic_cast<BattleUnit *>(board.tryGetUnitAt(p_cell)))
		{
			return unit;
		}
		for (const auto &unit : _storage)
		{
			if (unit->isDefeated() && unit->lastBoardPosition == p_cell)
			{
				return unit.get();
			}
		}
		return nullptr;
	}

	PlacedBattleObject *BattleContext::placeObject(
		std::string p_definitionId, const spk::Vector3Int &p_cell, int p_remainingTurns)
	{
		auto object = std::make_unique<PlacedBattleObject>(std::move(p_definitionId));
		if (!board.runtime().tryAdd(*object, p_cell, p_remainingTurns))
		{
			return nullptr;
		}
		PlacedBattleObject *result = object.get();
		_objectStorage.push_back(std::move(object));
		return result;
	}

	std::size_t BattleContext::removeObjects(
		const spk::Vector3Int &p_cell, const std::vector<std::string> &p_tags)
	{
		const auto before = board.runtime().getAt(p_cell);
		const std::size_t removed = board.runtime().removeByTags(p_cell, p_tags);
		if (removed == 0)
		{
			return 0;
		}
		_objectStorage.erase(std::remove_if(_objectStorage.begin(), _objectStorage.end(), [&](const auto &object) {
								 return std::ranges::find(before, object.get()) != before.end() &&
										std::ranges::all_of(p_tags, [&](const std::string &tag) {
											return object->hasTag(tag);
										});
							 }),
							 _objectStorage.end());
		return removed;
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
