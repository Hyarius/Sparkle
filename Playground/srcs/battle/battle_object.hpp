#pragma once

#include "battle/battle_side.hpp"

#include <string>
#include <string_view>
#include <utility>
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

	struct PlacedBattleObject final : BattleObject
	{
		std::string definitionId;
		explicit PlacedBattleObject(std::string p_definitionId) :
			definitionId(std::move(p_definitionId))
		{
			tags.push_back(definitionId);
		}
	};
}
