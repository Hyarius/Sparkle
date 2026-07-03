#pragma once

#include "encounters/encounter_types.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	class Actor;
	class BattleContext;
	class BattleUnit;
	struct BattleEvent;
	enum class BattleSide;

	struct EventCenter
	{
		spk::ContractProvider<Actor *, spk::Vector3Int> actorMoveRequested;
		spk::ContractProvider<spk::Vector3Int> playerMoved;
		spk::ContractProvider<spk::Vector3Int> invalidTarget;
		spk::ContractProvider<const EncounterSpawn &> encounterTriggered;
		spk::ContractProvider<BattleContext *> battleStarted;
		spk::ContractProvider<BattleUnit *> battleUnitPlaced;
		spk::ContractProvider<BattleContext *, BattleSide> battleResolved;
		spk::ContractProvider<> battleEndConfirmed;
		spk::ContractProvider<BattleUnit *> battleTurnEnded;
		spk::ContractProvider<const BattleEvent *> battleEventOccurred;

		// Step 14/18: featProgressUpdated when CreatureUnit exists.
		// Step 10/19: creatureImpressed when BattleUnit exists and taming lands.
		// Step 14/19: creatureRecruited when CreatureUnit exists and taming lands.
	};
}
