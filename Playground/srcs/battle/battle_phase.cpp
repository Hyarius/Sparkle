#include "battle/battle_phase.hpp"

namespace pg
{
	std::string_view toString(BattlePhase p_phase) noexcept
	{
		switch (p_phase)
		{
		case BattlePhase::Deployment:
			return "deployment";
		case BattlePhase::AwaitingActivation:
			return "awaitingActivation";
		case BattlePhase::Activation:
			return "activation";
		case BattlePhase::Terminal:
			return "terminal";
		}
		return "deployment";
	}

	std::string_view toString(BattleAbortReason p_reason) noexcept
	{
		switch (p_reason)
		{
		case BattleAbortReason::RequiredTerrainChanged:
			return "requiredTerrainChanged";
		case BattleAbortReason::SchedulerNoFutureProgress:
			return "schedulerNoFutureProgress";
		case BattleAbortReason::TimelineBoundaryMadeNoProgress:
			return "timelineBoundaryMadeNoProgress";
		case BattleAbortReason::NumericInvariant:
			return "numericInvariant";
		case BattleAbortReason::EffectChainDepthExceeded:
			return "effectChainDepthExceeded";
		case BattleAbortReason::InternalInvariantViolation:
			return "internalInvariantViolation";
		}
		return "internalInvariantViolation";
	}
}
