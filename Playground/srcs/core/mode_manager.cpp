#include "core/mode_manager.hpp"

#include "battle/battle_context.hpp"
#include "core/battle_mode.hpp"
#include "core/exploration_mode.hpp"
#include "encounters/encounter_emitter.hpp"

#include <stdexcept>
#include <utility>

namespace pg
{
	ModeManager::ModeManager(GameContext &p_context) :
		ModeManager(
			p_context,
			std::make_unique<ExplorationMode>(p_context),
			std::make_unique<BattleMode>(p_context))
	{
	}

	ModeManager::ModeManager(
		GameContext &p_context,
		std::unique_ptr<Mode> p_explorationMode,
		std::unique_ptr<Mode> p_battleMode) :
		_context(p_context),
		_explorationMode(std::move(p_explorationMode)),
		_battleMode(std::move(p_battleMode))
	{
		if (_explorationMode == nullptr || _battleMode == nullptr)
		{
			throw std::invalid_argument("ModeManager requires exploration and battle modes");
		}
		_battleStartedContract = _context.events.battleStarted.subscribe([this](BattleContext *p_context) {
			if (auto *mode = dynamic_cast<BattleMode *>(_battleMode.get()); mode != nullptr)
			{
				mode->setBattleContext(p_context);
				enterBattle();
			}
		});
		_battleResolvedContract = _context.events.battleResolved.subscribe([this](BattleContext *p_context, BattleSide p_winner) {
			if (_currentMode == _battleMode.get())
			{
				_resolvedContext = p_context;
				auto *mode = dynamic_cast<BattleMode *>(_battleMode.get());
				if (mode == nullptr || !mode->presentResult(p_winner))
				{
					_completeBattle();
				}
			}
		});
		_battleEndConfirmedContract = _context.events.battleEndConfirmed.subscribe([this] {
			if (_currentMode == _battleMode.get() && _resolvedContext != nullptr)
			{
				_completeBattle();
			}
		});
	}

	ModeManager::~ModeManager()
	{
		if (_currentMode != nullptr)
		{
			_currentMode->exit();
		}
	}

	void ModeManager::switchTo(Mode &p_mode)
	{
		if (&p_mode != _explorationMode.get() && &p_mode != _battleMode.get())
		{
			throw std::invalid_argument("ModeManager can only switch to an owned mode");
		}
		if (_currentMode == &p_mode)
		{
			return;
		}

		if (_currentMode != nullptr)
		{
			_currentMode->exit();
		}
		_currentMode = &p_mode;
		_currentMode->enter();
	}

	void ModeManager::enterExploration()
	{
		switchTo(*_explorationMode);
	}

	void ModeManager::enterBattle()
	{
		switchTo(*_battleMode);
	}

	void ModeManager::_completeBattle()
	{
		if (_resolvedContext != nullptr && _resolvedContext->returnWorldCell.has_value() &&
			_context.world.encounterEmitter != nullptr)
		{
			_context.world.encounterEmitter->suppressCell(*_resolvedContext->returnWorldCell);
		}
		_resolvedContext = nullptr;
		enterExploration();
	}

	void ModeManager::update(const spk::UpdateTick &p_tick)
	{
		if (_currentMode != nullptr)
		{
			_currentMode->update(p_tick);
		}
	}

	Mode *ModeManager::currentMode() noexcept
	{
		return _currentMode;
	}

	const Mode *ModeManager::currentMode() const noexcept
	{
		return _currentMode;
	}

	BattleMode *ModeManager::battleMode() noexcept
	{
		return dynamic_cast<BattleMode *>(_battleMode.get());
	}
}
