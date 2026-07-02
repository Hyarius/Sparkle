#include "core/exploration_mode.hpp"

#include <iostream>

namespace pg
{
	ExplorationMode::ExplorationMode(GameContext &p_context) :
		Mode(p_context)
	{
	}

	void ExplorationMode::enter()
	{
		std::cout << "ExplorationMode::enter" << std::endl;
		activate();
	}

	void ExplorationMode::exit()
	{
		deactivate();
		std::cout << "ExplorationMode::exit" << std::endl;
	}

	void ExplorationMode::activate()
	{
		_context.world.explorationActive = true;
	}

	void ExplorationMode::deactivate()
	{
		_context.world.explorationActive = false;
	}
}
