#include "battle/phases/battle_coordinator.hpp"

#include "battle/battle_context.hpp"
#include "battle/rules/battle_outcome_rules.hpp"
#include "battle/rules/battle_status_rules.hpp"
#include "battle/rules/battle_turn_rules.hpp"
#include "core/event_center.hpp"

#include <stdexcept>

namespace pg
{
	BattleCoordinator::BattleCoordinator(
		BattleContext &c, std::uint32_t seed, bool p_interactivePlacement) :
		_context(c),
		_setup([this] {
			_orchestrator.transitionTo(_placement);
		}),
		_placement(c, seed, p_interactivePlacement, [this](bool ok) {
			if (!ok)
			{
				throw std::runtime_error("Battle placement failed");
			}
			_orchestrator.transitionTo(_idle);
		}),
		_idle(c, [this](BattleUnit *u) {
			if (!u)
			{
				_finish();
				return;
			}
			if (u->side == BattleSide::Player)
			{
				_orchestrator.transitionTo(_player);
			}
			else
			{
				_orchestrator.transitionTo(_enemy);
			}
		}),
		_player([this](std::unique_ptr<BattleAction> a) {
			_onAction(std::move(a));
		}),
		_enemy(c, [this](std::unique_ptr<BattleAction> a) {
			_onAction(std::move(a));
		}),
		_resolution(c, [this](BattleActionKind k, bool ok) {
			_afterResolution(k, ok);
		}),
		_end([this] {
			_finished = true;
		})
	{
	}
	BattleCoordinator::~BattleCoordinator()
	{
		_orchestrator.reset();
	}
	void BattleCoordinator::start()
	{
		_orchestrator.transitionTo(_setup);
	}
	void BattleCoordinator::tick(float s)
	{
		BattleStatusRules::advanceSeconds(_context, s);
		_orchestrator.tick(s);
	}
	void BattleCoordinator::_onAction(std::unique_ptr<BattleAction> a)
	{
		if (!a)
		{
			return;
		}
		_resolution.setPending(std::move(a));
		_orchestrator.transitionTo(_resolution);
	}
	void BattleCoordinator::_afterResolution(BattleActionKind kind, bool success)
	{
		if (auto result = BattleOutcomeRules::winner(_context))
		{
			_winner = result;
			_finish();
			return;
		}
		if (!success || kind == BattleActionKind::EndTurn || !BattleTurnRules::canContinueTurn(_context))
		{
			BattleTurnRules::endTurn(_context);
			_orchestrator.transitionTo(_idle);
			return;
		}
		BattleUnit *u = _context.currentTurn.activeUnit;
		if (u->side == BattleSide::Player)
		{
			_orchestrator.transitionTo(_player);
		}
		else
		{
			_orchestrator.transitionTo(_enemy);
		}
	}
	void BattleCoordinator::_finish()
	{
		if (!_winner)
		{
			_winner = BattleOutcomeRules::winner(_context);
		}
		if (_winner)
		{
			for (BattleUnit *u : _context.getUnits(*_winner))
			{
				if (u->isActiveInBattle())
				{
					_context.report(BattleWonEvent{.context = {.turnIndex = _context.currentTurn.turnIndex, .caster = u}, .side = *_winner, .unitSurvived = true});
				}
			}
			_context.events().battleResolved.trigger(&_context, *_winner);
		}
		_orchestrator.transitionTo(_end);
	}
	PlayerTurnPhase &BattleCoordinator::playerTurnPhase() noexcept
	{
		return _player;
	}
	PlacementPhase &BattleCoordinator::placementPhase() noexcept
	{
		return _placement;
	}
	bool BattleCoordinator::finished() const noexcept
	{
		return _finished;
	}
	std::optional<BattleSide> BattleCoordinator::winner() const noexcept
	{
		return _winner;
	}
	std::string_view BattleCoordinator::currentPhaseName() const noexcept
	{
		return _orchestrator.current() ? _orchestrator.current()->name() : std::string_view{};
	}
}
