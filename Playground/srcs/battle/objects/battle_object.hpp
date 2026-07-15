#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_types.hpp"
#include "battle/effects/duration_state.hpp"
#include "board/board_cell.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct BattleObjectTriggerState
	{
		std::string triggerId;
		std::uint32_t timesTriggered = 0;
		[[nodiscard]] bool operator==(const BattleObjectTriggerState &) const = default;
	};

	struct BattleObjectInstance
	{
		BattleObjectId id;
		std::string definitionId;
		std::optional<BattleUnitId> creator;
		BattleSide creatorSide = BattleSide::Player;
		BoardCell cell;
		DurationState duration{InfiniteDurationState{}};
		std::vector<BattleObjectTriggerState> triggerStates;
		std::optional<std::string> sourceAbilityId;
		std::optional<std::string> sourceEffectId;
		[[nodiscard]] bool operator==(const BattleObjectInstance &) const = default;
	};

	struct BattleObjectSnapshot
	{
		BattleObjectId id;
		std::string definitionId;
		std::optional<BattleUnitId> creator;
		BattleSide creatorSide = BattleSide::Player;
		BoardCell cell;
		DurationSnapshot duration;
		std::vector<BattleObjectTriggerState> triggerStates;
		bool blocksMovement = false;
		bool blocksLineOfSight = false;
		[[nodiscard]] bool operator==(const BattleObjectSnapshot &) const = default;
	};
}
