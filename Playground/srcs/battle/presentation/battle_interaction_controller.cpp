#include "battle/presentation/battle_interaction_controller.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace
{
	[[nodiscard]] bool removed(const pg::BattleUnitSnapshot &p_unit) noexcept
	{
		return p_unit.removalReason != pg::RemovalReason::None;
	}
}

namespace pg
{
	BattleInteractionController::BattleInteractionController(
		BattleSession &p_commands,
		SnapshotProvider p_snapshots,
		StateChanged p_stateChanged) :
		_commands(p_commands),
		_snapshots(std::move(p_snapshots)),
		_stateChanged(std::move(p_stateChanged)),
		_snapshot(_snapshots())
	{
		syncFromSnapshot(_snapshot);
	}

	const BattleUnitSnapshot *BattleInteractionController::_unit(BattleUnitId p_id) const noexcept
	{
		const auto iterator = std::find_if(_snapshot.units.begin(), _snapshot.units.end(), [p_id](const BattleUnitSnapshot &unit) {
			return unit.id == p_id;
		});
		return iterator == _snapshot.units.end() ? nullptr : &*iterator;
	}

	std::optional<BattleUnitId> BattleInteractionController::_selectedPlacementUnit() const
	{
		const auto *placement = std::get_if<PlacementSelecting>(&_state);
		if (placement == nullptr || !placement->selected.has_value())
		{
			return std::nullopt;
		}
		for (const BattleUnitSnapshot &unit : _snapshot.units)
		{
			if (unit.side == BattleSide::Player && unit.persistentCreatureId == placement->selected && !removed(unit) && !unit.placed)
			{
				return unit.id;
			}
		}
		return std::nullopt;
	}

	bool BattleInteractionController::_isPlayerActivation(BattleUnitId p_id) const
	{
		const BattleUnitSnapshot *unit = _unit(p_id);
		return _snapshot.phase == BattlePhase::Activation && _snapshot.activeUnit == p_id && unit != nullptr &&
			   unit->side == BattleSide::Player && !removed(*unit) && unit->placed;
	}

	void BattleInteractionController::_notify()
	{
		if (_stateChanged)
		{
			_stateChanged();
		}
	}

	void BattleInteractionController::_reject(BoardCell p_cell)
	{
		_invalidCell = p_cell;
		_invalidSeconds = 0.3f;
		_notify();
	}

	InputHandling BattleInteractionController::_submit(BattleCommand p_command)
	{
		if (_submitting)
		{
			return {};
		}
		_submitting = true;
		_state = InteractionBusy{};
		_notify();
		CommandResult result = _commands.submit(p_command, {CommandController::Player});
		_submitting = false;
		_snapshot = _snapshots();
		if (const auto *rejected = std::get_if<RejectedCommand>(&result); rejected != nullptr && _hovered.has_value())
		{
			_reject(*_hovered);
		}
		syncFromSnapshot(_snapshot);
		return {.disposition = InputHandlingDisposition::Submitted, .submission = std::move(result)};
	}

	void BattleInteractionController::syncFromSnapshot(const BattleSnapshot &p_snapshot)
	{
		_snapshot = p_snapshot;
		if (_snapshot.phase == BattlePhase::Deployment && _snapshot.outcome == BattleOutcome::Undecided)
		{
			std::optional<CreatureInstanceId> retained;
			if (const auto *old = std::get_if<PlacementSelecting>(&_state); old != nullptr)
			{
				retained = old->selected;
			}
			if (retained.has_value() && !_selectedPlacementUnit().has_value())
			{
				retained.reset();
			}
			_state = PlacementSelecting{.selected = retained};
		}
		else if (_snapshot.phase == BattlePhase::Activation && _snapshot.activeUnit.has_value() && _isPlayerActivation(*_snapshot.activeUnit))
		{
			const bool retainMove = std::holds_alternative<SelectingMovement>(_state) &&
									std::get<SelectingMovement>(_state).unit == *_snapshot.activeUnit;
			const bool retainAbility = std::holds_alternative<SelectingAbility>(_state) &&
									   std::get<SelectingAbility>(_state).unit == *_snapshot.activeUnit;
			if (!retainMove && !retainAbility)
			{
				_state = AwaitingAction{*_snapshot.activeUnit};
			}
		}
		else
		{
			_state = InteractionInactive{};
			_hovered.reset();
		}
		_notify();
	}

	void BattleInteractionController::hover(std::optional<BoardCell> p_cell)
	{
		if (_hovered == p_cell)
		{
			return;
		}
		_hovered = p_cell;
		_notify();
	}

	void BattleInteractionController::update(float p_seconds)
	{
		if (_invalidSeconds <= 0.0f)
		{
			return;
		}
		_invalidSeconds -= std::max(0.0f, p_seconds);
		if (_invalidSeconds <= 0.0f)
		{
			_invalidSeconds = 0.0f;
			_invalidCell.reset();
			_notify();
		}
	}

	std::vector<BattleMaskCell> BattleInteractionController::overlayCandidates() const
	{
		std::vector<BattleMaskCell> cells;
		const auto add = [&cells](const std::vector<BoardCell> &values, BattleMaskKind kind) {
			for (const BoardCell &cell : values)
			{
				cells.push_back({.cell = cell, .kind = kind});
			}
		};
		const std::optional<BattleUnitId> selectedPlacementUnit = _selectedPlacementUnit();
		if (_snapshot.phase == BattlePhase::Deployment)
		{
			for (const BoardCell &cell : _commands.board().deployment().cells(BattleSide::Player))
			{
				// Make the available deployment strip discoverable before a card is selected.
				// Selection then applies the exact command preview, excluding cells unavailable to
				// that creature without asking presentation code to duplicate battle rules.
				if (!selectedPlacementUnit.has_value() || _commands.planPlacement(*selectedPlacementUnit, cell).has_value())
				{
					cells.push_back({.cell = cell, .kind = BattleMaskKind::Deployment});
				}
			}
		}
		if (const auto *movement = std::get_if<SelectingMovement>(&_state); movement != nullptr)
		{
			for (const MovePlan &plan : _commands.legalMoves(movement->unit))
			{
				cells.push_back({.cell = plan.destination, .kind = BattleMaskKind::Reachable});
			}
			if (_hovered.has_value())
			{
				if (const auto plan = _commands.planMove(movement->unit, *_hovered); plan.has_value())
				{
					for (const PlannedPathStep &step : plan->steps)
					{
						cells.push_back({.cell = step.to, .kind = BattleMaskKind::Path});
					}
				}
			}
		}
		if (const auto *ability = std::get_if<SelectingAbility>(&_state); ability != nullptr)
		{
			for (const AbilityAnchorPreview &preview : _commands.abilityAnchors(ability->unit, ability->abilityId))
			{
				if (preview.legal)
				{
					cells.push_back({.cell = preview.anchor, .kind = BattleMaskKind::AbilityRange});
				}
				else if (std::find(preview.reasons.begin(), preview.reasons.end(), CommandRejection::AnchorLineOfSightBlocked) != preview.reasons.end())
				{
					cells.push_back({.cell = preview.anchor, .kind = BattleMaskKind::LineOfSightBlocked});
				}
			}
			if (_hovered.has_value())
			{
				if (const auto plan = _commands.planCast(ability->unit, ability->abilityId, *_hovered); plan.has_value())
				{
					add(plan->affectedCells, BattleMaskKind::AreaPreview);
				}
			}
		}
		if (_hovered.has_value())
		{
			cells.push_back({.cell = *_hovered, .kind = BattleMaskKind::Hovered});
		}
		if (_invalidCell.has_value())
		{
			cells.push_back({.cell = *_invalidCell, .kind = BattleMaskKind::Invalid});
		}
		return cells;
	}

	const BattleInteractionState &BattleInteractionController::state() const noexcept
	{
		return _state;
	}

	const std::optional<BoardCell> &BattleInteractionController::hoveredCell() const noexcept
	{
		return _hovered;
	}

	std::string_view BattleInteractionController::stateName() const noexcept
	{
		return std::visit([](const auto &state) -> std::string_view {
			using State = std::decay_t<decltype(state)>;
			if constexpr (std::is_same_v<State, InteractionInactive>)
			{
				return "inactive";
			}
			if constexpr (std::is_same_v<State, InteractionBusy>)
			{
				return "busy";
			}
			if constexpr (std::is_same_v<State, PlacementSelecting>)
			{
				return "placementSelecting";
			}
			if constexpr (std::is_same_v<State, PlacementReady>)
			{
				return "placementReady";
			}
			if constexpr (std::is_same_v<State, AwaitingAction>)
			{
				return "awaitingAction";
			}
			if constexpr (std::is_same_v<State, SelectingMovement>)
			{
				return "selectingMovement";
			}
			return "selectingAbility";
		},
						  _state);
	}

	AbilitySlotAvailability BattleInteractionController::abilityAvailability(std::size_t p_visibleSlot) const
	{
		const auto *awaiting = std::get_if<AwaitingAction>(&_state);
		if (awaiting == nullptr || !_isPlayerActivation(awaiting->unit))
		{
			return {.enabled = false, .rejection = CommandRejection::WrongPhase};
		}
		const BattleUnitSnapshot *unit = _unit(awaiting->unit);
		if (unit == nullptr || p_visibleSlot >= unit->abilityIds.size())
		{
			return {.enabled = false, .rejection = CommandRejection::UnknownAbility};
		}
		const std::vector<AbilityAnchorPreview> previews = _commands.abilityAnchors(unit->id, unit->abilityIds[p_visibleSlot]);
		for (const AbilityAnchorPreview &preview : previews)
		{
			if (preview.legal)
			{
				return {.enabled = true};
			}
		}
		for (const AbilityAnchorPreview &preview : previews)
		{
			if (!preview.reasons.empty())
			{
				return {.enabled = false, .rejection = preview.reasons.front()};
			}
		}
		return {.enabled = false, .rejection = CommandRejection::CommandUnavailable};
	}

	AbilitySlotAvailability BattleInteractionController::deploymentReadyAvailability() const
	{
		if (!std::holds_alternative<PlacementSelecting>(_state))
		{
			return {.enabled = false, .rejection = CommandRejection::WrongPhase};
		}
		for (const BattleUnitSnapshot &unit : _snapshot.units)
		{
			if (unit.side == BattleSide::Player && unit.removalReason == RemovalReason::None && !unit.placed)
			{
				return {.enabled = false, .rejection = CommandRejection::DeploymentIncomplete};
			}
		}
		return {.enabled = true};
	}

	InputHandling BattleInteractionController::selectPlacementCreature(CreatureInstanceId p_creature)
	{
		if (!std::holds_alternative<PlacementSelecting>(_state))
		{
			return {};
		}
		for (const BattleUnitSnapshot &unit : _snapshot.units)
		{
			if (unit.side == BattleSide::Player && unit.persistentCreatureId == p_creature && !removed(unit) && !unit.placed)
			{
				std::get<PlacementSelecting>(_state).selected = p_creature;
				_notify();
				return {.disposition = InputHandlingDisposition::Handled};
			}
		}
		return {};
	}

	InputHandling BattleInteractionController::selectPlacementRosterSlot(std::size_t p_rosterSlot)
	{
		if (!std::holds_alternative<PlacementSelecting>(_state))
		{
			return {};
		}
		for (const BattleUnitSnapshot &unit : _snapshot.units)
		{
			if (unit.side == BattleSide::Player && unit.rosterOrder == p_rosterSlot &&
				unit.persistentCreatureId.has_value() && !removed(unit) && !unit.placed)
			{
				return selectPlacementCreature(*unit.persistentCreatureId);
			}
		}
		return {};
	}

	InputHandling BattleInteractionController::clickBoardCell(BoardCell p_cell)
	{
		if (const std::optional<BattleUnitId> unit = _selectedPlacementUnit(); unit.has_value())
		{
			if (const auto plan = _commands.planPlacement(*unit, p_cell); plan.has_value())
			{
				return _submit(*plan);
			}
			_reject(p_cell);
			return {.disposition = InputHandlingDisposition::Handled};
		}
		if (const auto *movement = std::get_if<SelectingMovement>(&_state); movement != nullptr)
		{
			const BattleUnitSnapshot *unit = _unit(movement->unit);
			if (unit != nullptr && unit->cell == p_cell)
			{
				return cancel();
			}
			if (const auto plan = _commands.planMove(movement->unit, p_cell); plan.has_value())
			{
				return _submit(MoveCommand{.unit = movement->unit, .destination = plan->destination});
			}
			_reject(p_cell);
			return {.disposition = InputHandlingDisposition::Handled};
		}
		if (const auto *ability = std::get_if<SelectingAbility>(&_state); ability != nullptr)
		{
			if (const auto plan = _commands.planCast(ability->unit, ability->abilityId, p_cell); plan.has_value())
			{
				return _submit(CastAbilityCommand{.unit = ability->unit, .abilityId = ability->abilityId, .anchor = plan->anchor});
			}
			_reject(p_cell);
			return {.disposition = InputHandlingDisposition::Handled};
		}
		return {};
	}

	InputHandling BattleInteractionController::selectMove()
	{
		const auto *awaiting = std::get_if<AwaitingAction>(&_state);
		if (awaiting == nullptr || !_isPlayerActivation(awaiting->unit))
		{
			return {};
		}
		_state = SelectingMovement{awaiting->unit};
		_notify();
		return {.disposition = InputHandlingDisposition::Handled};
	}

	InputHandling BattleInteractionController::selectAbility(std::size_t p_visibleSlot)
	{
		const auto *awaiting = std::get_if<AwaitingAction>(&_state);
		if (awaiting == nullptr || !_isPlayerActivation(awaiting->unit))
		{
			return {};
		}
		const BattleUnitSnapshot *unit = _unit(awaiting->unit);
		if (unit == nullptr || p_visibleSlot >= unit->abilityIds.size())
		{
			return {};
		}
		const std::string &abilityId = unit->abilityIds[p_visibleSlot];
		const std::vector<AbilityAnchorPreview> previews = _commands.abilityAnchors(unit->id, abilityId);
		std::vector<const AbilityAnchorPreview *> legal;
		for (const AbilityAnchorPreview &preview : previews)
		{
			if (preview.legal && preview.plan.has_value())
			{
				legal.push_back(&preview);
			}
		}
		if (legal.empty())
		{
			return {.disposition = InputHandlingDisposition::Handled};
		}
		if (legal.size() == 1 && unit->cell.has_value() && legal.front()->anchor == *unit->cell)
		{
			return _submit(CastAbilityCommand{.unit = unit->id, .abilityId = abilityId, .anchor = legal.front()->anchor});
		}
		_state = SelectingAbility{.unit = unit->id, .abilityId = abilityId};
		_notify();
		return {.disposition = InputHandlingDisposition::Handled};
	}

	InputHandling BattleInteractionController::cancel()
	{
		if (const auto *movement = std::get_if<SelectingMovement>(&_state); movement != nullptr)
		{
			_state = AwaitingAction{movement->unit};
			_notify();
			return {.disposition = InputHandlingDisposition::Handled};
		}
		if (const auto *ability = std::get_if<SelectingAbility>(&_state); ability != nullptr)
		{
			_state = AwaitingAction{ability->unit};
			_notify();
			return {.disposition = InputHandlingDisposition::Handled};
		}
		if (auto *placement = std::get_if<PlacementSelecting>(&_state); placement != nullptr && placement->selected.has_value())
		{
			placement->selected.reset();
			_notify();
			return {.disposition = InputHandlingDisposition::Handled};
		}
		return {};
	}

	InputHandling BattleInteractionController::confirmDeployment()
	{
		if (!std::holds_alternative<PlacementSelecting>(_state))
		{
			return {};
		}
		return _submit(ConfirmDeploymentCommand{});
	}

	InputHandling BattleInteractionController::endTurn()
	{
		const auto *awaiting = std::get_if<AwaitingAction>(&_state);
		if (awaiting == nullptr || !_isPlayerActivation(awaiting->unit))
		{
			return {};
		}
		return _submit(EndTurnCommand{.unit = awaiting->unit, .cause = EndTurnRequestCause::Explicit});
	}
}
