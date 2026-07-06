#pragma once

#include "battle/battle_side.hpp"

#include <cstdint>
namespace pg
{
	class BattleContext;
	class BattlePlacementRules
	{
	public:
		[[nodiscard]] static bool autoPlaceSide(
			BattleContext &p_context, BattleSide p_side, std::uint32_t p_seed);
		static void clearSide(BattleContext &p_context, BattleSide p_side);
		[[nodiscard]] static bool autoPlace(BattleContext &p_context, std::uint32_t p_seed);
	};
}
