#pragma once

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	struct EventCenter
	{
		// Step 07: actorMoveRequested(Actor &, spk::Vector3Int) when Actor exists.
		spk::ContractProvider<spk::Vector3Int> playerMoved;

		// Step 08: encounterTriggered(const EncounterSpawn &) when EncounterSpawn exists.
		// Step 10: battleStarted, battleResolved, battleTurnEnded and battleEventOccurred
		// when the battle payload types exist.
		// Step 14/18: featProgressUpdated when CreatureUnit exists.
		// Step 10/19: creatureImpressed when BattleUnit exists and taming lands.
		// Step 14/19: creatureRecruited when CreatureUnit exists and taming lands.
	};
}
