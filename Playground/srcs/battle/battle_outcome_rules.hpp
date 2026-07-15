#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_phase.hpp"
#include "battle/battle_types.hpp"

#include <optional>

namespace pg
{
	class BattleContext;

	struct BattleTerminalRecord
	{
		BattleId battleId;
		BattleOutcome outcome = BattleOutcome::Undecided;
		std::optional<BattleAbortReason> abortReason;

		[[nodiscard]] bool operator==(const BattleTerminalRecord &) const = default;
	};

	[[nodiscard]] BattleOutcome evaluateBattleOutcome(const BattleContext &p_context) noexcept;
}
