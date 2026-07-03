#include "board/board_runtime_registry.hpp"

namespace pg
{
	bool BoardRuntimeRegistry::canRegister(const BattleObject &p_unit, const spk::Vector3Int &p_cell) const noexcept
	{
		return !_unitPositions.contains(&p_unit) && !_unitsByCell.contains(p_cell);
	}

	bool BoardRuntimeRegistry::tryRegister(BattleObject &p_unit, const spk::Vector3Int &p_cell)
	{
		if (!canRegister(p_unit, p_cell))
		{
			return false;
		}
		_unitsByCell.emplace(p_cell, &p_unit);
		_unitPositions.emplace(&p_unit, p_cell);
		return true;
	}

	bool BoardRuntimeRegistry::tryMove(BattleObject &p_unit, const spk::Vector3Int &p_to)
	{
		const auto position = _unitPositions.find(&p_unit);
		if (position == _unitPositions.end() || _unitsByCell.contains(p_to))
		{
			return false;
		}
		_unitsByCell.erase(position->second);
		position->second = p_to;
		_unitsByCell.emplace(p_to, &p_unit);
		return true;
	}

	bool BoardRuntimeRegistry::swapUnits(BattleObject &p_first, BattleObject &p_second)
	{
		if (&p_first == &p_second)
		{
			return _unitPositions.contains(&p_first);
		}
		const auto first = _unitPositions.find(&p_first);
		const auto second = _unitPositions.find(&p_second);
		if (first == _unitPositions.end() || second == _unitPositions.end())
		{
			return false;
		}
		const spk::Vector3Int firstPosition = first->second;
		const spk::Vector3Int secondPosition = second->second;
		first->second = secondPosition;
		second->second = firstPosition;
		_unitsByCell[firstPosition] = &p_second;
		_unitsByCell[secondPosition] = &p_first;
		return true;
	}

	bool BoardRuntimeRegistry::remove(BattleObject &p_unit) noexcept
	{
		const auto position = _unitPositions.find(&p_unit);
		if (position == _unitPositions.end())
		{
			return false;
		}
		_unitsByCell.erase(position->second);
		_unitPositions.erase(position);
		return true;
	}

	std::optional<spk::Vector3Int> BoardRuntimeRegistry::tryGetPosition(const BattleObject &p_unit) const noexcept
	{
		const auto position = _unitPositions.find(&p_unit);
		return position == _unitPositions.end() ? std::nullopt : std::optional<spk::Vector3Int>(position->second);
	}

	BattleObject *BoardRuntimeRegistry::tryGetUnitAt(const spk::Vector3Int &p_cell) const noexcept
	{
		const auto unit = _unitsByCell.find(p_cell);
		return unit == _unitsByCell.end() ? nullptr : unit->second;
	}

	bool BoardRuntimeRegistry::hasUnitAt(const spk::Vector3Int &p_cell) const noexcept
	{
		return _unitsByCell.contains(p_cell);
	}

	bool BoardRuntimeRegistry::tryAdd(BattleObject &p_object, const spk::Vector3Int &p_cell, int p_remainingTurns)
	{
		if (_objectPositions.contains(&p_object) || p_remainingTurns == 0 || p_remainingTurns < -1)
		{
			return false;
		}
		_objectsByCell[p_cell].push_back({&p_object, p_remainingTurns});
		_objectPositions.emplace(&p_object, p_cell);
		return true;
	}

	std::vector<BattleObject *> BoardRuntimeRegistry::getAt(const spk::Vector3Int &p_cell) const
	{
		std::vector<BattleObject *> result;
		const auto objects = _objectsByCell.find(p_cell);
		if (objects == _objectsByCell.end())
		{
			return result;
		}
		result.reserve(objects->second.size());
		for (const ObjectEntry &entry : objects->second)
		{
			result.push_back(entry.object);
		}
		return result;
	}

	std::size_t BoardRuntimeRegistry::removeByTags(
		const spk::Vector3Int &p_cell,
		const std::vector<std::string> &p_tags)
	{
		const auto objects = _objectsByCell.find(p_cell);
		if (objects == _objectsByCell.end() || p_tags.empty())
		{
			return 0;
		}
		std::size_t removed = 0;
		auto &entries = objects->second;
		for (auto iterator = entries.begin(); iterator != entries.end();)
		{
			const bool matches = std::ranges::all_of(p_tags, [iterator](const std::string &p_tag) {
				return iterator->object->hasTag(p_tag);
			});
			if (!matches)
			{
				++iterator;
				continue;
			}
			_objectPositions.erase(iterator->object);
			iterator = entries.erase(iterator);
			++removed;
		}
		if (entries.empty())
		{
			_objectsByCell.erase(objects);
		}
		return removed;
	}

	std::vector<BattleObject *> BoardRuntimeRegistry::advanceObjectDurations()
	{
		std::vector<BattleObject *> expired;
		for (auto cell = _objectsByCell.begin(); cell != _objectsByCell.end();)
		{
			auto &entries = cell->second;
			for (auto entry = entries.begin(); entry != entries.end();)
			{
				if (entry->remainingTurns > 0)
				{
					--entry->remainingTurns;
				}
				if (entry->remainingTurns != 0)
				{
					++entry;
					continue;
				}
				expired.push_back(entry->object);
				_objectPositions.erase(entry->object);
				entry = entries.erase(entry);
			}
			cell = entries.empty() ? _objectsByCell.erase(cell) : std::next(cell);
		}
		return expired;
	}
}
