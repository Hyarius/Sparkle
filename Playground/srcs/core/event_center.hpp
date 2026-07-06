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
	};
}
