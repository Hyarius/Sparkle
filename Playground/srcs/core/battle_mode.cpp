#include "core/battle_mode.hpp"

#include <iostream>

namespace pg
{
	BattleMode::BattleMode(GameContext &p_context) :
		Mode(p_context)
	{
	}

	void BattleMode::enter()
	{
		std::cout << "BattleMode::enter" << std::endl;
		activate();
	}

	void BattleMode::exit()
	{
		deactivate();
		std::cout << "BattleMode::exit" << std::endl;
	}

	void BattleMode::activate()
	{
		// Steps 10 and 11 activate battle orchestration, input, camera, HUD, and overlays.
	}

	void BattleMode::deactivate()
	{
		// Steps 10 and 11 deactivate battle orchestration, input, camera, HUD, and overlays.
	}
}
