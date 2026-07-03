#pragma once

#include "battle/battle_side.hpp"
#include <optional>
namespace pg
{
	class BattleContext;
	class BattleOutcomeRules
	{
	public:
		[[nodiscard]] static std::optional<BattleSide> winner(const BattleContext &p_context);
	};
}
