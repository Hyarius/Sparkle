#pragma once

#include <string_view>

namespace pg
{
	// The lifecycle a single battle walks through. Step 06 only ever leaves it in Deployment or
	// Terminal: Deployment while both sides are seated, Terminal once a technical abort ends it.
	// AwaitingActivation and Activation are declared now so the event/snapshot vocabulary is final,
	// but step 07 is what first moves the phase off Deployment on an ordinary (non-abort) path.
	enum class BattlePhase
	{
		Deployment,
		AwaitingActivation,
		Activation,
		Terminal
	};

	// The closed set of reasons a battle ends technically rather than by an ordinary outcome. A
	// construction/command validation error, a user closing the window and ordinary teardown are
	// NOT here: those are expected values, not aborted battles.
	//
	// Step 06 can only reach RequiredTerrainChanged (a pinned live chunk moved under the session)
	// and InternalInvariantViolation (a checked-counter reserve failure). The rest are placed here
	// so steps 07-10 extend behaviour without widening the enum.
	enum class BattleAbortReason
	{
		RequiredTerrainChanged,
		SchedulerNoFutureProgress,
		TimelineBoundaryMadeNoProgress,
		NumericInvariant,
		EffectChainDepthExceeded,
		InternalInvariantViolation
	};

	[[nodiscard]] std::string_view toString(BattlePhase p_phase) noexcept;
	[[nodiscard]] std::string_view toString(BattleAbortReason p_reason) noexcept;
}
