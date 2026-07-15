#pragma once

#include "battle/battle_command.hpp"
#include "battle/battle_command_result.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_event.hpp"
#include "battle/battle_event_log.hpp"
#include "battle/battle_outcome_rules.hpp"
#include "battle/battle_snapshot.hpp"
#include "battle/battle_unit.hpp"
#include "battle/query/battle_plans.hpp"
#include "battle/scheduler/scheduler_result.hpp"
#include "board/board_data.hpp"
#include "encounters/encounter_definition.hpp" // OpponentPlacementPolicy

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace pg
{
	class Registries;

	// Everything session creation needs beyond the immutable registries and the already-built board:
	// the result-facing descriptor, one value seed per actual participant (built by the caller from
	// the shared step-04 derivation), and the authored enemy placement policy step 12 read off the
	// resolved encounter. The session validates all of it before it publishes anything.
	struct BattleConstructionRequest
	{
		BattleDescriptor descriptor;
		std::vector<BattleParticipantSeed> participants;
		OpponentPlacementPolicy enemyPlacement;
	};

	// The sole public mutation facade for one battle. It owns a private BattleContext and never
	// exposes a mutable reference to it: presentation, AI and tests submit value commands and read
	// copied snapshots/events. Live-world and handcrafted boards flow through the identical path.
	class BattleSession
	{
	private:
		std::unique_ptr<BattleContext> _context;
		bool _resolving = false; // re-entry guard for a future synchronous subscriber

		explicit BattleSession(std::unique_ptr<BattleContext> p_context) noexcept;

		[[nodiscard]] CommandResult _placeUnit(const PlaceUnitCommand &p_command, CommandIssuer p_issuer);
		[[nodiscard]] CommandResult _confirmDeployment(const ConfirmDeploymentCommand &p_command, CommandIssuer p_issuer);
		[[nodiscard]] CommandResult _endTurn(const EndTurnCommand &p_command, CommandIssuer p_issuer);
		[[nodiscard]] CommandResult _move(const MoveCommand &p_command, CommandIssuer p_issuer);
		[[nodiscard]] CommandResult _cast(const CastAbilityCommand &p_command, CommandIssuer p_issuer);
		[[nodiscard]] CommandResult _abort(BattleAbortReason p_reason);
		void _assertAcceptedInvariants() const;

	public:
		~BattleSession();
		BattleSession(const BattleSession &) = delete;
		BattleSession &operator=(const BattleSession &) = delete;

		// Transactionally builds the whole session or returns a structured error with no partial
		// session, event, RNG state or board occupancy published. Enemy auto-deployment and the
		// Construction batch happen inside this call.
		[[nodiscard]] static BattleConstructionResult create(
			BattleConstructionRequest p_request,
			const Registries &p_registries,
			BoardData p_board);

		// The one gateway. Validation is pure and complete before any mutation; a rejected command
		// changes nothing and draws no RNG; an accepted command commits exactly one before/after
		// batch; a pre-commit technical failure discards its scratch and commits one no-action
		// TechnicalAbort batch instead.
		[[nodiscard]] CommandResult submit(const BattleCommand &p_command, CommandIssuer p_issuer);
		[[nodiscard]] SchedulerCallResult advanceUntilActivation();

		[[nodiscard]] BattleSnapshot snapshot() const;
		[[nodiscard]] std::vector<BattleEvent> copyEvents(EventRange p_range) const;
		[[nodiscard]] std::vector<CommittedBattleBatch> takeCommittedBatches();
		[[nodiscard]] const std::vector<CommittedBattleBatch> &archivedBatches() const noexcept;

		// The deterministic material-state digest of the whole battle (section 15).
		[[nodiscard]] std::uint64_t gameplayProgressDigest() const;

		[[nodiscard]] BattleId battleId() const noexcept;
		[[nodiscard]] BattlePhase phase() const noexcept;
		[[nodiscard]] BattleOutcome outcome() const noexcept;
		[[nodiscard]] const std::optional<BattleTerminalRecord> &terminalRecord() const noexcept;
		[[nodiscard]] std::vector<MovePlan> legalMoves(BattleUnitId p_unit) const;
		[[nodiscard]] std::vector<AbilityAnchorPreview> abilityAnchors(BattleUnitId p_unit, std::string_view p_abilityId) const;

		// Test seam (section 18.5): drive the checked sequence counters near their maximum so the next
		// ordinary command must roll back into the reserved TechnicalAbort batch. Not production.
		void primeSequenceCountersForExhaustionTest(
			std::uint64_t p_nextAction,
			std::uint64_t p_nextBatch,
			std::uint64_t p_nextEvent);
		// Test-only: forces the next shield allocation to overflow, exercising cast rollback.
		void primeShieldIdsForExhaustionTest(std::uint32_t p_next);
	};
}
