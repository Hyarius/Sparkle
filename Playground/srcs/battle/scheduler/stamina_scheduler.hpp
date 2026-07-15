#pragma once

#include "battle/scheduler/scheduler_result.hpp"

namespace pg
{
	class BattleContext;

	class StaminaScheduler
	{
	public:
		[[nodiscard]] static SchedulerAdvanceResult advanceUntilActivation(BattleContext &p_context);
	};
}
