#pragma once

#include "battle/battle_log.hpp"
#include "battle/battle_unit.hpp"
#include "battle/turn_context.hpp"
#include "board/board_data.hpp"
#include "encounters/encounter_types.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace pg
{
	struct EventCenter;

	class BattleContext
	{
	private:
		EventCenter &_events;
		std::vector<std::unique_ptr<BattleUnit>> _storage;
		std::vector<BattleUnit *> _playerUnits;
		std::vector<BattleUnit *> _enemyUnits;

	public:
		BoardData board;
		PlacementStyle placementStyle = PlacementStyle::Random;
		std::optional<spk::Vector3Int> returnWorldCell;
		TurnContext currentTurn;
		bool allowsTaming = false;
		BattleLog log;

		BattleContext(EventCenter &p_events, BoardData p_board);
		BattleUnit &addUnit(BattleUnitSource p_source, BattleSide p_side);
		[[nodiscard]] const std::vector<BattleUnit *> &getUnits(BattleSide p_side) const;
		[[nodiscard]] std::vector<BattleUnit *> getOpponents(const BattleUnit &p_unit) const;
		[[nodiscard]] std::vector<BattleUnit *> allUnits() const;
		[[nodiscard]] bool hasLivingUnits(BattleSide p_side) const;
		[[nodiscard]] bool tryPlaceUnit(BattleUnit &p_unit, const spk::Vector3Int &p_cell);
		[[nodiscard]] bool tryMoveUnit(BattleUnit &p_unit, const spk::Vector3Int &p_cell);
		[[nodiscard]] bool trySwapUnits(BattleUnit &p_first, BattleUnit &p_second);
		[[nodiscard]] bool removeUnit(BattleUnit &p_unit);
		[[nodiscard]] bool defeatUnit(BattleUnit &p_unit);
		void report(BattleEvent p_event);
		[[nodiscard]] EventCenter &events() noexcept;
	};
}
