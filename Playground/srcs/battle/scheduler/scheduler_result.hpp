#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_sequence_ids.hpp"

#include <optional>
#include <variant>
#include <vector>

namespace pg
{
	enum class SchedulerStop
	{
		ActivationReady,
		Terminal,
		Aborted
	};
	enum class SchedulerRejection
	{
		WrongPhase,
		SessionBusy
	};

	struct SchedulerAdvanceResult
	{
		SchedulerStop stop = SchedulerStop::Terminal;
		std::optional<BattleUnitId> activeUnit;
		std::vector<BattleBatchId> committedBatches;
	};
	struct RejectedSchedulerAdvance
	{
		SchedulerRejection reason = SchedulerRejection::WrongPhase;
	};
	using SchedulerCallResult = std::variant<SchedulerAdvanceResult, RejectedSchedulerAdvance>;
}
