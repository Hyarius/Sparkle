#pragma once

#include "core/event_center.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	struct WorldContext
	{
		int seed = 0;
	};

	struct PlayerData
	{
		spk::Vector3 position = spk::Vector3::Zero;
	};

	struct GameContext
	{
		EventCenter events;
		WorldContext world;
		PlayerData player;

		GameContext();
	};
}
