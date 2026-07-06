#include "battle/phases/battle_phases.hpp"

#include "ai/ai_evaluator.hpp"
#include "ai/ai_behaviour.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_resolver.hpp"
#include "battle/rules/battle_outcome_rules.hpp"
#include "battle/rules/battle_placement_rules.hpp"
#include "battle/rules/battle_turn_rules.hpp"
#include "core/event_center.hpp"

#include <algorithm>

namespace pg
{
	void SetupPhase::enter()
	{
		_done();
	}
	void PlacementPhase::enter()
	{
		_active = true;
		_selected = nullptr;
		if (!_interactivePlayer)
		{
			_done(BattlePlacementRules::autoPlace(_context, _seed));
			return;
		}
		if (!BattlePlacementRules::autoPlaceSide(_context, BattleSide::Enemy, _seed))
		{
			_done(false);
			return;
		}
		BattlePlacementRules::clearSide(_context, BattleSide::Player);
		_changed.trigger();
	}

	void PlacementPhase::exit()
	{
		_active = false;
		_selected = nullptr;
		_changed.trigger();
	}

	bool PlacementPhase::select(BattleUnit *p_unit)
	{
		if (!_active || !_interactivePlayer || (p_unit != nullptr &&
			(p_unit->side != BattleSide::Player ||
			 std::ranges::find(_context.getUnits(BattleSide::Player), p_unit) ==
				 _context.getUnits(BattleSide::Player).end())))
		{
			return false;
		}
		_selected = p_unit;
		_changed.trigger();
		return true;
	}

	bool PlacementPhase::placeSelected(const spk::Vector3Int &p_cell)
	{
		if (_selected == nullptr || !_active || !_interactivePlayer ||
			std::ranges::find(_context.board.deploymentZones().player, p_cell) ==
				_context.board.deploymentZones().player.end())
		{
			return false;
		}

		BattleUnit *selected = _selected;
		auto *occupant = dynamic_cast<BattleUnit *>(_context.board.tryGetUnitAt(p_cell));
		bool placed = false;
		if (occupant == selected)
		{
			return true;
		}
		if (occupant != nullptr)
		{
			if (occupant->side != BattleSide::Player)
			{
				return false;
			}
			if (selected->boardPosition.has_value())
			{
				placed = _context.trySwapUnits(*selected, *occupant);
			}
			else
			{
				const spk::Vector3Int occupiedCell = *occupant->boardPosition;
				(void)_context.removeUnit(*occupant);
				placed = _context.tryPlaceUnit(*selected, p_cell);
				if (!placed)
				{
					(void)_context.tryPlaceUnit(*occupant, occupiedCell);
				}
				else
				{
					_selected = occupant;
				}
			}
		}
		else if (selected->boardPosition.has_value())
		{
			placed = _context.tryMoveUnit(*selected, p_cell);
		}
		else
		{
			placed = _context.tryPlaceUnit(*selected, p_cell);
		}

		if (placed)
		{
			_context.events().battlePlacementChanged.trigger();
			_changed.trigger();
		}
		return placed;
	}

	bool PlacementPhase::autoPlacePlayer()
	{
		if (!_active || !_interactivePlayer ||
			!BattlePlacementRules::autoPlaceSide(_context, BattleSide::Player, _seed))
		{
			return false;
		}
		_selected = nullptr;
		_context.events().battlePlacementChanged.trigger();
		_changed.trigger();
		return true;
	}

	bool PlacementPhase::confirm()
	{
		if (!canConfirm())
		{
			return false;
		}
		_done(true);
		return true;
	}

	bool PlacementPhase::canConfirm() const noexcept
	{
		return _active && std::ranges::all_of(
			_context.getUnits(BattleSide::Player), [](const BattleUnit *p_unit) {
				return p_unit != nullptr && p_unit->boardPosition.has_value();
			});
	}

	bool PlacementPhase::active() const noexcept { return _active; }
	bool PlacementPhase::interactivePlayer() const noexcept { return _interactivePlayer; }
	BattleUnit *PlacementPhase::selected() const noexcept { return _selected; }
	PlacementPhase::ChangeContract PlacementPhase::subscribeToChange(std::function<void()> p_callback)
	{
		return _changed.subscribe(std::move(p_callback));
	}
	void IdlePhase::enter()
	{
		if (BattleOutcomeRules::winner(_context).has_value())
		{
			_ready(nullptr);
			return;
		}
		BattleUnit *unit = nullptr;
		for (std::size_t guard = 0; guard <= _context.allUnits().size(); ++guard)
		{
			BattleTurnRules::advanceToNextReady(_context);
			unit = BattleTurnRules::selectReady(_context);
			if (unit == nullptr)
			{
				break;
			}
			BattleTurnRules::beginTurn(_context, *unit);
			if (unit->isActiveInBattle())
			{
				break;
			}
			unit = nullptr;
			if (BattleOutcomeRules::winner(_context).has_value())
			{
				break;
			}
		}
		_ready(unit);
	}
	bool PlayerTurnPhase::submitAction(std::unique_ptr<BattleAction> p_action)
	{
		if (!_active || !p_action)
		{
			return false;
		}
		_chosen(std::move(p_action));
		return true;
	}
	void EnemyTurnPhase::enter()
	{
		BattleUnit *unit = _context.currentTurn.activeUnit;
		if (unit == nullptr)
		{
			_chosen(nullptr);
			return;
		}
		_chosen(unit->aiBehaviour != nullptr
			? AIEvaluator::evaluate(*unit->aiBehaviour, *unit, _context)
			: std::make_unique<EndTurnAction>(*unit));
	}
	void ResolutionPhase::enter()
	{
		if (!_pending)
		{
			_done(BattleActionKind::EndTurn, false);
			return;
		}
		const BattleActionKind kind = _pending->kind();
		const bool result = BattleActionResolver::resolve(_context, *_pending);
		_done(kind, result);
	}
}
