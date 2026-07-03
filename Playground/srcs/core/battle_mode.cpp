#include "core/battle_mode.hpp"

#include "battle/battle_context.hpp"
#include "battle/phases/battle_coordinator.hpp"
#include "core/battle_scene.hpp"

#include <iostream>

namespace pg
{
	BattleMode::BattleMode(GameContext &p_context) :
		Mode(p_context)
	{
	}
	BattleMode::~BattleMode() = default;

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
		if (_battleContext != nullptr)
		{
			_coordinator = std::make_unique<BattleCoordinator>(*_battleContext, static_cast<std::uint32_t>(_context.world.seed));
			// Bind presentation before start() so the overlay/camera exist as the FSM runs Setup ->
			// Placement -> ... -> PlayerTurn synchronously.
			if (_scene != nullptr)
			{
				_scene->begin(*_battleContext, *_coordinator);
			}
			_coordinator->start();
			if (_scene != nullptr)
			{
				_scene->synchronizeUnits();
			}
		}
	}

	void BattleMode::deactivate()
	{
		// Note: the coordinator is NOT reset here. battleResolved fires from inside coordinator->tick
		// (via update()), and exit()/deactivate() runs on the resulting mode switch — freeing the
		// coordinator mid-tick would be use-after-free. It is replaced on the next activate().
		if (_scene != nullptr)
		{
			_scene->end();
		}
	}

	void BattleMode::update(const spk::UpdateTick &p_tick)
	{
		if (_coordinator != nullptr)
		{
			_coordinator->tick(static_cast<float>(p_tick.deltaTime.seconds()));
		}
		if (_scene != nullptr)
		{
			_scene->update(static_cast<float>(p_tick.deltaTime.seconds()));
		}
	}

	void BattleMode::setBattleContext(BattleContext *p_context) noexcept
	{
		_battleContext = p_context;
	}
	void BattleMode::setScene(BattleScene *p_scene) noexcept
	{
		_scene = p_scene;
	}
	BattleCoordinator *BattleMode::coordinator() noexcept
	{
		return _coordinator.get();
	}

	bool BattleMode::presentResult(BattleSide p_winner)
	{
		if (p_winner != BattleSide::Player)
		{
			std::cout << "Battle lost; player position unchanged (respawn comes with saves, step 32)" << std::endl;
		}
		if (_scene == nullptr)
		{
			return false;
		}
		_scene->showResult(p_winner, [this] {
			_context.events.battleEndConfirmed.trigger();
		});
		return true;
	}
}
