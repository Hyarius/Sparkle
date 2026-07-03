#include "battle/rules/battle_outcome_rules.hpp"
#include "battle/battle_context.hpp"
namespace pg
{
	std::optional<BattleSide> BattleOutcomeRules::winner(const BattleContext &p_context)
	{
		const bool p = p_context.hasLivingUnits(BattleSide::Player), e = p_context.hasLivingUnits(BattleSide::Enemy);
		if (p && e)
		{
			return std::nullopt;
		}
		if (p)
		{
			return BattleSide::Player;
		}
		if (e)
		{
			return BattleSide::Enemy;
		}
		return BattleSide::Neutral;
	}
}
