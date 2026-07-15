#pragma once

#include "battle/ai/battle_ai_driver.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class BattleSession;

	enum class BattlePumpState
	{
		NeedsDeployment,
		WaitingForPlayer,
		WaitingForAI,
		Progressed,
		Terminal
	};

	struct BattlePumpResult
	{
		BattlePumpState state = BattlePumpState::Progressed;
		std::size_t logicalOperations = 0;
		std::vector<AICommandDiagnostic> diagnostics;
	};

	// Headless scheduler/AI orchestration.  A call consumes only scheduler boundaries and ordinary
	// AI command submissions; it never accepts a frame delta and never mutates player control.
	class BattlePump
	{
	private:
		BattleAIDriver _driver;
		std::optional<std::string> _debugPlayerAutoplayBehaviour;

	public:
		void setDebugPlayerAutoplayBehaviour(std::optional<std::string> p_behaviourId)
		{
			_debugPlayerAutoplayBehaviour = std::move(p_behaviourId);
		}
		[[nodiscard]] const std::optional<std::string> &debugPlayerAutoplayBehaviour() const noexcept
		{
			return _debugPlayerAutoplayBehaviour;
		}
		[[nodiscard]] BattleAIDriver &driver() noexcept
		{
			return _driver;
		}
		[[nodiscard]] const BattleAIDriver &driver() const noexcept
		{
			return _driver;
		}

		[[nodiscard]] BattlePumpResult advanceUntilPlayerInput(BattleSession &p_session, std::size_t p_maxLogicalOperations);
	};
}
