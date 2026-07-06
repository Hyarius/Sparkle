#pragma once

#include "battle/phases/i_battle_phase.hpp"
namespace pg
{
	class BattleOrchestrator
	{
		IBattlePhase *_current = nullptr;

	public:
		~BattleOrchestrator();
		void reset();
		void transitionTo(IBattlePhase &p_phase);
		void tick(float p_seconds);
		[[nodiscard]] IBattlePhase *current() const noexcept;
	};
}
