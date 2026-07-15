#pragma once

#include "battle/battle_session.hpp"
#include "battle/presentation/battle_overlay_model.hpp"

#include <functional>
#include <optional>
#include <variant>

namespace pg
{
	struct InteractionInactive
	{
	};
	struct InteractionBusy
	{
	};
	struct PlacementSelecting
	{
		std::optional<CreatureInstanceId> selected;
	};
	struct PlacementReady
	{
	};
	struct AwaitingAction
	{
		BattleUnitId unit;
	};
	struct SelectingMovement
	{
		BattleUnitId unit;
	};
	struct SelectingAbility
	{
		BattleUnitId unit;
		std::string abilityId;
	};

	using BattleInteractionState = std::variant<
		InteractionInactive,
		InteractionBusy,
		PlacementSelecting,
		PlacementReady,
		AwaitingAction,
		SelectingMovement,
		SelectingAbility>;

	enum class InputHandlingDisposition
	{
		Ignored,
		Handled,
		Submitted
	};

	struct InputHandling
	{
		InputHandlingDisposition disposition = InputHandlingDisposition::Ignored;
		std::optional<CommandResult> submission;
	};

	struct AbilitySlotAvailability
	{
		bool enabled = false;
		std::optional<CommandRejection> rejection;
	};

	class BattleInteractionController
	{
	public:
		using SnapshotProvider = std::function<BattleSnapshot()>;
		using StateChanged = std::function<void()>;

	private:
		BattleSession &_commands;
		SnapshotProvider _snapshots;
		StateChanged _stateChanged;
		BattleSnapshot _snapshot;
		BattleInteractionState _state{InteractionInactive{}};
		std::optional<BoardCell> _hovered;
		std::optional<BoardCell> _invalidCell;
		float _invalidSeconds = 0.0f;
		bool _submitting = false;

		[[nodiscard]] const BattleUnitSnapshot *_unit(BattleUnitId p_id) const noexcept;
		[[nodiscard]] std::optional<BattleUnitId> _selectedPlacementUnit() const;
		[[nodiscard]] bool _isPlayerActivation(BattleUnitId p_id) const;
		void _notify();
		void _reject(BoardCell p_cell);
		[[nodiscard]] InputHandling _submit(BattleCommand p_command);

	public:
		BattleInteractionController(BattleSession &p_commands, SnapshotProvider p_snapshots, StateChanged p_stateChanged = {});

		void syncFromSnapshot(const BattleSnapshot &p_snapshot);
		void hover(std::optional<BoardCell> p_cell);
		void update(float p_seconds);
		[[nodiscard]] std::vector<BattleMaskCell> overlayCandidates() const;

		[[nodiscard]] const BattleInteractionState &state() const noexcept;
		[[nodiscard]] const std::optional<BoardCell> &hoveredCell() const noexcept;
		[[nodiscard]] std::string_view stateName() const noexcept;
		[[nodiscard]] AbilitySlotAvailability abilityAvailability(std::size_t p_visibleSlot) const;
		[[nodiscard]] AbilitySlotAvailability deploymentReadyAvailability() const;

		InputHandling selectPlacementCreature(CreatureInstanceId p_creature);
		InputHandling selectPlacementRosterSlot(std::size_t p_rosterSlot);
		InputHandling clickBoardCell(BoardCell p_cell);
		InputHandling selectMove();
		InputHandling selectAbility(std::size_t p_visibleSlot);
		InputHandling cancel();
		InputHandling confirmDeployment();
		InputHandling endTurn();
	};
}
