#pragma once

#include "battle/battle_lifecycle.hpp"
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
		// Lifecycle/publication notifications are value-owned.  A listener can retain a notice after
		// the runtime that emitted it has been torn down without holding a dangling BattleSession.
		spk::ContractProvider<BattleLifecycleNotice> battleLifecycle;
		spk::ContractProvider<BattleBatchPublication> battleBatchPublished;
	};
}
