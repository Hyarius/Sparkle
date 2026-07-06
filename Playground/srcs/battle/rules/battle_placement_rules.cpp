#include "battle/rules/battle_placement_rules.hpp"
#include "battle/battle_context.hpp"

#include <algorithm>
#include <random>

namespace pg
{
	void BattlePlacementRules::clearSide(BattleContext &p_context, BattleSide p_side)
	{
		for (BattleUnit *unit : p_context.getUnits(p_side))
		{
			if (unit != nullptr && unit->boardPosition.has_value())
			{
				(void)p_context.removeUnit(*unit);
			}
		}
	}

	bool BattlePlacementRules::autoPlaceSide(
		BattleContext &p_context, BattleSide p_side, std::uint32_t p_seed)
	{
		auto cells = p_side == BattleSide::Player
			? p_context.board.deploymentZones().player
			: p_context.board.deploymentZones().enemy;
		std::mt19937 random(p_seed);
		if (p_side == BattleSide::Enemy && p_context.placementStyle == PlacementStyle::Random)
		{
			std::shuffle(cells.begin(), cells.end(), random);
		}
		const auto &units = p_context.getUnits(p_side);
		if (cells.size() < units.size())
		{
			return false;
		}
		clearSide(p_context, p_side);
		for (std::size_t index = 0; index < units.size(); ++index)
		{
			if (units[index] == nullptr || !p_context.tryPlaceUnit(*units[index], cells[index]))
			{
				clearSide(p_context, p_side);
				return false;
			}
		}
		return true;
	}

	bool BattlePlacementRules::autoPlace(BattleContext &p_context, std::uint32_t p_seed)
	{
		return autoPlaceSide(p_context, BattleSide::Player, p_seed) &&
			autoPlaceSide(p_context, BattleSide::Enemy, p_seed);
	}
}
