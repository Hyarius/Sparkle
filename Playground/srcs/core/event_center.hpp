#pragma once

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	class Actor;

	struct EventCenter
	{
		spk::ContractProvider<Actor *, spk::Vector3Int> actorMoveRequested;
		spk::ContractProvider<spk::Vector3Int> playerMoved;
		spk::ContractProvider<spk::Vector3Int> invalidTarget;

		// Step 08: encounterTriggered(const EncounterSpawn &) when EncounterSpawn exists.
		// Step 10: battleStarted, battleResolved, battleTurnEnded and battleEventOccurred
		// when the battle payload types exist.
		// Step 14/18: featProgressUpdated when CreatureUnit exists.
		// Step 10/19: creatureImpressed when BattleUnit exists and taming lands.
		// Step 14/19: creatureRecruited when CreatureUnit exists and taming lands.
	};
}
