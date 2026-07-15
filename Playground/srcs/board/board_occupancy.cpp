#include "board/board_occupancy.hpp"

#include <algorithm>

namespace pg
{
	namespace
	{
		const std::vector<BattleObjectId> NoObjects;
	}

	BoardMutationResult BoardMutationResult::ok(bool p_changed) noexcept
	{
		return BoardMutationResult{.accepted = true, .changed = p_changed, .error = std::nullopt};
	}

	BoardMutationResult BoardMutationResult::rejected(BoardMutationError p_error) noexcept
	{
		return BoardMutationResult{.accepted = false, .changed = false, .error = p_error};
	}

	BoardOccupancy::BoardOccupancy(const TraversalGraph &p_navigation)
	{
		for (const TraversalGraph::Node &node : p_navigation.allNodes())
		{
			_standable.insert(node.position);
		}
	}

	bool BoardOccupancy::isStandable(BoardCell p_cell) const noexcept
	{
		return _standable.contains(p_cell);
	}

	std::optional<BattleUnitId> BoardOccupancy::unitAt(BoardCell p_cell) const noexcept
	{
		const auto found = _unitByCell.find(p_cell);
		return found == _unitByCell.end() ? std::nullopt : std::optional<BattleUnitId>(found->second);
	}

	std::optional<BoardCell> BoardOccupancy::cellOf(BattleUnitId p_unit) const noexcept
	{
		const auto found = _cellByUnit.find(p_unit);
		return found == _cellByUnit.end() ? std::nullopt : std::optional<BoardCell>(found->second);
	}

	std::span<const BattleObjectId> BoardOccupancy::objectsAt(BoardCell p_cell) const noexcept
	{
		const auto found = _objectsByCell.find(p_cell);
		return found == _objectsByCell.end() ? std::span<const BattleObjectId>(NoObjects)
											 : std::span<const BattleObjectId>(found->second);
	}

	std::optional<BoardCell> BoardOccupancy::cellOf(BattleObjectId p_object) const noexcept
	{
		const auto found = _cellByObject.find(p_object);
		return found == _cellByObject.end() ? std::nullopt : std::optional<BoardCell>(found->second);
	}

	std::size_t BoardOccupancy::unitCount() const noexcept
	{
		return _cellByUnit.size();
	}

	std::size_t BoardOccupancy::objectCount() const noexcept
	{
		return _cellByObject.size();
	}

	bool BoardOccupancy::invariantsHold() const noexcept
	{
		if (_unitByCell.size() != _cellByUnit.size())
		{
			return false;
		}
		for (const auto &[cell, unit] : _unitByCell)
		{
			const auto reverse = _cellByUnit.find(unit);
			if (reverse == _cellByUnit.end() || reverse->second != cell || !isStandable(cell) || !unit.valid())
			{
				return false;
			}
		}

		std::size_t stacked = 0;
		for (const auto &[cell, objects] : _objectsByCell)
		{
			if (objects.empty() || !isStandable(cell))
			{
				return false;
			}
			if (!std::ranges::is_sorted(objects) || std::ranges::adjacent_find(objects) != objects.end())
			{
				return false;
			}
			for (const BattleObjectId object : objects)
			{
				const auto reverse = _cellByObject.find(object);
				if (reverse == _cellByObject.end() || reverse->second != cell || !object.valid())
				{
					return false;
				}
			}
			stacked += objects.size();
		}
		return stacked == _cellByObject.size();
	}

	BoardMutationResult BoardOccupancy::placeUnit(BattleUnitId p_unit, BoardCell p_cell)
	{
		if (!p_unit.valid())
		{
			return BoardMutationResult::rejected(BoardMutationError::InvalidId);
		}
		if (!isStandable(p_cell))
		{
			return BoardMutationResult::rejected(BoardMutationError::DestinationNotStandable);
		}

		if (const auto placed = _cellByUnit.find(p_unit); placed != _cellByUnit.end())
		{
			// Placing a unit exactly where it already stands is an idempotent no-op; placing it
			// anywhere else is a move, and a move is not what the caller asked for.
			return placed->second == p_cell
					   ? BoardMutationResult::ok(false)
					   : BoardMutationResult::rejected(BoardMutationError::UnitAlreadyPlacedElsewhere);
		}
		if (_unitByCell.contains(p_cell))
		{
			return BoardMutationResult::rejected(BoardMutationError::DestinationOccupied);
		}

		_unitByCell.emplace(p_cell, p_unit);
		_cellByUnit.emplace(p_unit, p_cell);
		return BoardMutationResult::ok(true);
	}

	BoardMutationResult BoardOccupancy::moveUnit(BattleUnitId p_unit, BoardCell p_destination)
	{
		if (!p_unit.valid())
		{
			return BoardMutationResult::rejected(BoardMutationError::InvalidId);
		}
		const auto placed = _cellByUnit.find(p_unit);
		if (placed == _cellByUnit.end())
		{
			return BoardMutationResult::rejected(BoardMutationError::UnknownUnit);
		}
		if (!isStandable(p_destination))
		{
			return BoardMutationResult::rejected(BoardMutationError::DestinationNotStandable);
		}
		if (placed->second == p_destination)
		{
			return BoardMutationResult::ok(false);
		}
		if (_unitByCell.contains(p_destination))
		{
			return BoardMutationResult::rejected(BoardMutationError::DestinationOccupied);
		}

		_unitByCell.erase(placed->second);
		_unitByCell.emplace(p_destination, p_unit);
		placed->second = p_destination;
		return BoardMutationResult::ok(true);
	}

	BoardMutationResult BoardOccupancy::swapUnits(BattleUnitId p_first, BattleUnitId p_second)
	{
		if (!p_first.valid() || !p_second.valid())
		{
			return BoardMutationResult::rejected(BoardMutationError::InvalidId);
		}
		if (p_first == p_second)
		{
			return BoardMutationResult::rejected(BoardMutationError::CannotSwapSameUnit);
		}

		const auto first = _cellByUnit.find(p_first);
		const auto second = _cellByUnit.find(p_second);
		if (first == _cellByUnit.end() || second == _cellByUnit.end())
		{
			return BoardMutationResult::rejected(BoardMutationError::UnknownUnit);
		}

		// Both cells are already standable and already occupied by exactly these two units, so the
		// swap cannot half-fail: both maps are rewritten from values that are known good.
		const BoardCell firstCell = first->second;
		const BoardCell secondCell = second->second;
		first->second = secondCell;
		second->second = firstCell;
		_unitByCell[firstCell] = p_second;
		_unitByCell[secondCell] = p_first;
		return BoardMutationResult::ok(true);
	}

	bool BoardOccupancy::removeUnit(BattleUnitId p_unit) noexcept
	{
		const auto placed = _cellByUnit.find(p_unit);
		if (!p_unit.valid() || placed == _cellByUnit.end())
		{
			return false;
		}
		_unitByCell.erase(placed->second);
		_cellByUnit.erase(placed);
		return true;
	}

	BoardMutationResult BoardOccupancy::placeObject(BattleObjectId p_object, BoardCell p_cell)
	{
		if (!p_object.valid())
		{
			return BoardMutationResult::rejected(BoardMutationError::InvalidId);
		}
		if (!isStandable(p_cell))
		{
			return BoardMutationResult::rejected(BoardMutationError::DestinationNotStandable);
		}

		if (const auto placed = _cellByObject.find(p_object); placed != _cellByObject.end())
		{
			return placed->second == p_cell
					   ? BoardMutationResult::ok(false)
					   : BoardMutationResult::rejected(BoardMutationError::ObjectAlreadyPlacedElsewhere);
		}

		// Objects stack: a cell holding a trap and a wall holds both. They are kept in ascending id
		// order so objectsAt() is a canonical sequence rather than an insertion-order accident.
		std::vector<BattleObjectId> &stack = _objectsByCell[p_cell];
		stack.insert(std::ranges::upper_bound(stack, p_object), p_object);
		_cellByObject.emplace(p_object, p_cell);
		return BoardMutationResult::ok(true);
	}

	bool BoardOccupancy::removeObject(BattleObjectId p_object) noexcept
	{
		const auto placed = _cellByObject.find(p_object);
		if (!p_object.valid() || placed == _cellByObject.end())
		{
			return false;
		}

		const auto stacked = _objectsByCell.find(placed->second);
		if (stacked != _objectsByCell.end())
		{
			std::erase(stacked->second, p_object);
			if (stacked->second.empty())
			{
				_objectsByCell.erase(stacked);
			}
		}
		_cellByObject.erase(placed);
		return true;
	}
}
