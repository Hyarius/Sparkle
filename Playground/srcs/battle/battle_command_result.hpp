#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_phase.hpp" // BattleAbortReason
#include "battle/battle_sequence_ids.hpp"
#include "battle/battle_types.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace pg
{
	class BattleSession;

	// ---- Construction result ---------------------------------------------------------------------

	enum class BattleConstructionErrorCode
	{
		InvalidDescriptor,
		InvalidParticipantCount,
		DuplicateParticipantIdentity,
		InvalidRosterOrder,
		UnknownDefinitionReference,
		InvalidDerivedLoadout,
		BoardSourceMismatch,
		InsufficientDeploymentCapacity,
		EnemyPlacementInvalid,
		NumericOverflow
	};

	// An expected value, not an exception: a bad descriptor, a missing definition or a board that
	// cannot seat both teams is content/timing, so it is reported with the code plus the exact
	// descriptor/side/roster context and publishes no partial session. Allocation/process-invariant
	// exceptions are never flattened into one of these codes.
	struct BattleConstructionError
	{
		BattleConstructionErrorCode code = BattleConstructionErrorCode::InvalidDescriptor;
		std::string encounterDefinitionId;
		std::optional<BattleSide> side;
		std::optional<std::uint32_t> rosterOrder;
		std::optional<BattleUnitId> unitId;
		std::string referencedId;
		std::string diagnosticDetail;
	};

	[[nodiscard]] std::string_view toString(BattleConstructionErrorCode p_code) noexcept;

	// Exactly one of session/error is populated. Move-only; the destructor is defined out of line
	// once BattleSession is a complete type, so no unique_ptr deletion is instantiated here.
	struct BattleConstructionResult
	{
		std::unique_ptr<BattleSession> session;
		std::optional<BattleConstructionError> error;

		explicit BattleConstructionResult(std::unique_ptr<BattleSession> p_session);
		explicit BattleConstructionResult(BattleConstructionError p_error);
		BattleConstructionResult(const BattleConstructionResult &) = delete;
		BattleConstructionResult &operator=(const BattleConstructionResult &) = delete;
		BattleConstructionResult(BattleConstructionResult &&) noexcept;
		BattleConstructionResult &operator=(BattleConstructionResult &&) noexcept;
		~BattleConstructionResult();

		[[nodiscard]] bool succeeded() const noexcept { return session != nullptr; }
	};

	// ---- Command result --------------------------------------------------------------------------

	// A half-open range of committed event sequence numbers: [first, onePastLast).
	struct EventRange
	{
		BattleEventSequence first;
		BattleEventSequence onePastLast;

		[[nodiscard]] bool operator==(const EventRange &) const = default;
		[[nodiscard]] std::uint64_t count() const noexcept
		{
			return onePastLast.value - first.value;
		}
	};

	enum class CommandRejection
	{
		SessionBusy,
		BattleTerminal,
		WrongPhase,
		WrongController,
		UnknownUnit,
		UnitRemoved,
		UnitAlreadyConfirmed,
		UnitOutsideDeploymentZone,
		DestinationNotStandable,
		DestinationOccupied,
		DeploymentIncomplete,
		CommandUnavailable,
		NoStateChange,
		UnitNotPlaced,
		NotActiveUnit,
		UnknownBoardCell,
		UnknownAbility,
		AbilityNotOwned,
		InsufficientActionPoints,
		InsufficientMovementPoints,
		DestinationBlocked,
		DestinationUnreachable,
		AnchorOutOfRange,
		AnchorLineOfSightBlocked,
		AnchorFilterRejected,
		EffectRuntimeUnavailable
		// steps 07-08 append resource/range/path/target errors here
	};

	[[nodiscard]] std::string_view toString(CommandRejection p_reason) noexcept;

	// A rejected command changed no unit/board/phase/flag/counter, advanced no RNG draw, and
	// appended/committed/published no event or empty batch.
	struct RejectedCommand
	{
		CommandRejection reason = CommandRejection::CommandUnavailable;

		[[nodiscard]] bool operator==(const RejectedCommand &) const = default;
	};

	struct AcceptedCommand
	{
		BattleActionId actionId;
		BattleBatchId batchId;
		EventRange events;

		[[nodiscard]] bool operator==(const AcceptedCommand &) const = default;
	};

	// Not a rejection: a pre-commit technical invariant failed while submit was resolving, the
	// uncommitted command transaction was discarded, and one no-action TechnicalAbort batch was
	// committed from the last trustworthy state. It consumes no action id.
	struct AbortedCommand
	{
		BattleAbortReason reason = BattleAbortReason::InternalInvariantViolation;
		BattleBatchId batchId;
		EventRange events;

		[[nodiscard]] bool operator==(const AbortedCommand &) const = default;
	};

	using CommandResult = std::variant<AcceptedCommand, RejectedCommand, AbortedCommand>;
}
