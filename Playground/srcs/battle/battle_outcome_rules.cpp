#include "battle/battle_outcome_rules.hpp"

#include "battle/battle_context.hpp"

namespace pg
{
	BattleOutcome evaluateBattleOutcome(const BattleContext &p_context) noexcept
	{
		const int players = p_context.activeCount(BattleSide::Player);
		const int enemies = p_context.activeCount(BattleSide::Enemy);
		if (players > 0 && enemies == 0)
		{
			return BattleOutcome::PlayerVictory;
		}
		if (players == 0 && enemies > 0)
		{
			return BattleOutcome::PlayerDefeat;
		}
		if (players == 0 && enemies == 0)
		{
			return BattleOutcome::Draw;
		}
		return BattleOutcome::Undecided;
	}
}
