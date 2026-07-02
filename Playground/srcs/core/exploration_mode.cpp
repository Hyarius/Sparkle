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
		// Step 07 activates exploration input, camera control, and encounter emission.
	}

	void ExplorationMode::deactivate()
	{
		// Step 07 deactivates exploration input, camera control, and encounter emission.
	}
}
