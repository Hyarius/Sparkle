#pragma once

#include "battle/battle_side.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	struct BattleObject
	{
		BattleSide side = BattleSide::Neutral;
		std::vector<std::string> tags;
		virtual ~BattleObject() = default;

		[[nodiscard]] bool hasTag(std::string_view p_tag) const;
	};
}
