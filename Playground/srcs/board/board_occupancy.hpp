#pragma once

#include "battle/battle_ids.hpp"
#include "board/board_cell.hpp"
#include "board/traversal_graph.hpp"

#include <map>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace pg
{
	enum class BoardMutationError
	{
		InvalidId,
		UnknownUnit,
		UnknownObject,
		DestinationNotStandable,
		DestinationOccupied,
		UnitAlreadyPlacedElsewhere,
		ObjectAlreadyPlacedElsewhere,
		CannotSwapSameUnit
	};

	// accepted is true exactly when error is empty. An accepted result with changed == false is a
	// successful no-op: placing a unit where it already stands is idempotent, not a duplicate. A
	// rejected one changed nothing at all - no mutation method leaves a half-applied map behind.
	struct BoardMutationResult
	{
		bool accepted = false;
		bool changed = false;
		std::optional<BoardMutationError> error;

		[[nodiscard]] static BoardMutationResult ok(bool p_changed) noexcept;
		[[nodiscard]] static BoardMutationResult rejected(BoardMutationError p_error) noexcept;

		[[nodiscard]] bool operator==(const BoardMutationResult &) const = default;
	};

	// Who stands where, in stable step-01 ids: never a BattleUnit*, a component pointer, a vector
	// index or a display name, any of which would make a replay depend on this run's addresses.
	//
	// Units and objects are separate spaces: exactly one unit may stand on a standable cell, while
	// any number of battle objects may stack on one. An object does not block movement merely by
	// existing - step 10 reads its immutable blocksMovement definition when it builds the movement
	// blocker - so occupancy stores objects without pretending they are units.
	//
	// Standability is fixed for the board's lifetime, so it is captured once from the immutable
	// navigation graph rather than re-derived: the set below and the graph are two views of the
	// same frozen topology, and both stop changing when the board is built.
	class BoardOccupancy
	{
	private:
		std::set<BoardCell, BoardCellLess> _standable;
		std::map<BoardCell, BattleUnitId, BoardCellLess> _unitByCell;
		std::map<BattleUnitId, BoardCell> _cellByUnit;
		std::map<BoardCell, std::vector<BattleObjectId>, BoardCellLess> _objectsByCell;
		std::map<BattleObjectId, BoardCell> _cellByObject;

	public:
		BoardOccupancy() = default;
		explicit BoardOccupancy(const TraversalGraph &p_navigation);

		[[nodiscard]] bool isStandable(BoardCell p_cell) const noexcept;

		[[nodiscard]] std::optional<BattleUnitId> unitAt(BoardCell p_cell) const noexcept;
		[[nodiscard]] std::optional<BoardCell> cellOf(BattleUnitId p_unit) const noexcept;
		// Ascending id order, always: a caller may iterate the stack without sorting it, and no
		// container's iteration order leaks into a rule.
		[[nodiscard]] std::span<const BattleObjectId> objectsAt(BoardCell p_cell) const noexcept;
		[[nodiscard]] std::optional<BoardCell> cellOf(BattleObjectId p_object) const noexcept;

		[[nodiscard]] std::size_t unitCount() const noexcept;
		[[nodiscard]] std::size_t objectCount() const noexcept;

		// The forward and reverse maps agree, every occupied cell is standable, and every object
		// stack is sorted and duplicate-free. Tests assert this after every operation; production
		// code has no reason to call it.
		[[nodiscard]] bool invariantsHold() const noexcept;

		// The internal mutation surface. Step 06 hands it to BattleContext and the command
		// resolver through BoardData::mutation(); UI, AI and snapshot code get const access only.
		// Its existence is not permission to bypass the single command path.
		[[nodiscard]] BoardMutationResult placeUnit(BattleUnitId p_unit, BoardCell p_cell);
		[[nodiscard]] BoardMutationResult moveUnit(BattleUnitId p_unit, BoardCell p_destination);
		[[nodiscard]] BoardMutationResult swapUnits(BattleUnitId p_first, BattleUnitId p_second);
		[[nodiscard]] bool removeUnit(BattleUnitId p_unit) noexcept;
		[[nodiscard]] BoardMutationResult placeObject(BattleObjectId p_object, BoardCell p_cell);
		[[nodiscard]] bool removeObject(BattleObjectId p_object) noexcept;
	};
}
