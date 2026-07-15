#pragma once

#include "battle/battle_context.hpp"
#include "battle_objects/battle_object_definition.hpp"
#include "statuses/status_definition.hpp"

#include <optional>

namespace pg
{
	// Explicit reaction dispatcher.  It is never an event subscriber: callers choose the exact
	// insertion point and the guard makes a reaction's own effects observationally shallow.
	class StatusHookService
	{
	public:
		static void dispatchStatusHook(
			BattleContext &, std::vector<StagedEvent> &, BattleUnitId p_owner, StatusHook, std::optional<BattleUnitId> p_primary = std::nullopt, std::optional<BattleStatusInstanceId> p_onlyInstance = std::nullopt);
		static void dispatchRemovedHook(
			BattleContext &, std::vector<StagedEvent> &, BattleUnitId p_owner, const BattleStatusInstance &p_removed);
		static void dispatchObjectTrigger(
			BattleContext &, std::vector<StagedEvent> &, BattleObjectTrigger, BattleUnitId p_unit, BoardCell p_cell);
	};
}
