#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_types.hpp"
#include "board/board_cell.hpp"

#include <string>
#include <variant>

namespace pg
{
	// The complete value-command vocabulary, declared in full now so steps 07-08 extend behaviour
	// without replacing the gateway. Every command is a copied value: no pointer, no path, no
	// component, so a replay input is exactly the bytes that were submitted.

	struct PlaceUnitCommand
	{
		BattleUnitId unit;
		BoardCell destination;

		[[nodiscard]] bool operator==(const PlaceUnitCommand &) const = default;
	};

	struct ConfirmDeploymentCommand
	{
		BattleSide side = BattleSide::Player;

		[[nodiscard]] bool operator==(const ConfirmDeploymentCommand &) const = default;
	};

	// Intentionally no caller-supplied path, distance or MP cost: step 08 recomputes all three
	// authoritatively from the board so a client can never assert a cheaper move than the terrain
	// allows.
	struct MoveCommand
	{
		BattleUnitId unit;
		BoardCell destination;

		[[nodiscard]] bool operator==(const MoveCommand &) const = default;
	};

	// Exactly one anchor. Area cells and affected targets are resolved internally once (step 08);
	// a client never enumerates them.
	struct CastAbilityCommand
	{
		BattleUnitId unit;
		std::string abilityId;
		BoardCell anchor;

		[[nodiscard]] bool operator==(const CastAbilityCommand &) const = default;
	};

	// Only Player UI submits Explicit; AI/autoplay may submit one of the four safety causes. The
	// automatic activation-end reasons (no legal command, cap, defeat, ...) are deliberately NOT
	// representable in a submitted command, so UI/AI/replay cannot spoof them.
	enum class EndTurnRequestCause
	{
		Explicit,
		AiRepeatedStateGuard,
		AiCommandCap,
		AiInvalidPlannedCommand,
		AiNoRuleProducedCommand
	};

	// The closed reason an activation ended, mapped one-to-one from an accepted request cause plus
	// the internal automatic causes. Declared now; step 07 emits it.
	enum class ActivationEndReason
	{
		Explicit,
		NoLegalCommands,
		CommandCap,
		ActiveUnitDefeated,
		ActiveUnitImpressed,
		BattleTerminal,
		TechnicalAbort,
		AiRepeatedStateGuard,
		AiCommandCap,
		AiInvalidPlannedCommand,
		AiNoRuleProducedCommand
	};

	struct EndTurnCommand
	{
		BattleUnitId unit;
		EndTurnRequestCause cause = EndTurnRequestCause::Explicit;

		[[nodiscard]] bool operator==(const EndTurnCommand &) const = default;
	};

	using BattleCommand =
		std::variant<PlaceUnitCommand, ConfirmDeploymentCommand, MoveCommand, CastAbilityCommand, EndTurnCommand>;

	// Who is issuing a command. The gateway validates the controller against the command and the
	// unit's side before allocating any id or touching any state.
	enum class CommandController
	{
		System,        // transactional setup / enemy deployment / internal lifecycle only
		Player,        // normal battle UI; Player-side units only
		EnemyAi,       // Enemy-side activation commands only
		DebugAutoplay  // Player-side control in an explicitly configured development session
	};

	struct CommandIssuer
	{
		CommandController controller = CommandController::Player;

		[[nodiscard]] bool operator==(const CommandIssuer &) const = default;
	};

	[[nodiscard]] std::string_view toString(CommandController p_controller) noexcept;
}
