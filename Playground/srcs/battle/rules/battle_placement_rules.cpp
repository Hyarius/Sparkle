#include "battle/rules/battle_placement_rules.hpp"
#include "battle/battle_context.hpp"

#include <algorithm>
#include <random>

namespace pg
{
	bool BattlePlacementRules::autoPlace(BattleContext &p_context, std::uint32_t p_seed)
	{
		auto playerCells = p_context.board.deploymentZones().player;
		auto enemyCells = p_context.board.deploymentZones().enemy;
		std::mt19937 random(p_seed);
		if (p_context.placementStyle == PlacementStyle::Random)
		{
			std::shuffle(enemyCells.begin(), enemyCells.end(), random);
		}
		const auto &players = p_context.getUnits(BattleSide::Player);
		const auto &enemies = p_context.getUnits(BattleSide::Enemy);
		if (playerCells.size() < players.size() || enemyCells.size() < enemies.size())
		{
			return false;
		}
		for (std::size_t i = 0; i < players.size(); ++i)
		{
			if (!p_context.tryPlaceUnit(*players[i], playerCells[i]))
			{
				return false;
			}
		}
		for (std::size_t i = 0; i < enemies.size(); ++i)
		{
			if (!p_context.tryPlaceUnit(*enemies[i], enemyCells[i]))
			{
				return false;
			}
		}
		return true;
	}
}
