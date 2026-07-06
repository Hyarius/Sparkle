#pragma once

#include "encounters/encounter_types.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector3.hpp"

#include <string>

namespace pg
{
	class Actor;
	class BattleContext;
	class BattleUnit;
	class CreatureUnit;
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
		spk::ContractProvider<> battlePlacementChanged;
		spk::ContractProvider<BattleContext *, BattleSide> battleResolved;
		spk::ContractProvider<> battleEndConfirmed;
		spk::ContractProvider<BattleUnit *> battleTurnEnded;
		spk::ContractProvider<const BattleEvent *> battleEventOccurred;
		spk::ContractProvider<CreatureUnit *, int> featProgressUpdated;
		spk::ContractProvider<BattleUnit *> creatureImpressed;
		spk::ContractProvider<CreatureUnit *> creatureRecruited;
		spk::ContractProvider<> worldChanged;
		spk::ContractProvider<> partyChanged;
		spk::ContractProvider<std::string> interactionPromptChanged;
		spk::ContractProvider<bool> explorationModeChanged;

	};
}
