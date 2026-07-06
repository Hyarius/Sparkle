#include "core/exploration_mode.hpp"

#include "encounters/encounter_emitter.hpp"

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
		_context.events.explorationModeChanged.trigger(true);
		if (_context.world.encounterEmitter != nullptr)
		{
			_context.world.encounterEmitter->setEnabled(true);
		}
	}

	void ExplorationMode::deactivate()
	{
		if (_context.world.encounterEmitter != nullptr)
		{
			_context.world.encounterEmitter->setEnabled(false);
		}
		_context.world.explorationActive = false;
		_context.events.explorationModeChanged.trigger(false);
	}
}
