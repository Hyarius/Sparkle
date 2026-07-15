#pragma once

#include "battle/battle_command.hpp"
#include "battle/battle_command_result.hpp"
#include "battle/battle_event_log.hpp" // brings BattleEvent, BattleSnapshot, CommittedBattleBatch
#include "battle/battle_ids.hpp"
#include "battle/battle_outcome_rules.hpp"
#include "battle/battle_rng.hpp"
#include "battle/battle_types.hpp"
#include "battle/battle_unit.hpp"
#include "board/board_data.hpp"

#include "structures/math/spk_vector3.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class Registries;

	// Immutable result-facing provenance, copied once at construction and never guessed later. Every
	// terminal result, transcript, diagnostic header and persistence notice copies boardSource from
	// here; no consumer infers the source from encounter kind, seed, origin or a definition lookup.
	struct BattleDescriptor
	{
		BattleId battleId;
		std::string stableEncounterIdentity;
		std::string encounterDefinitionId;
		std::string encounterDisplayName;
		EncounterKind encounterKind = EncounterKind::Wild;
		bool allowsTaming = false;
		bool repeatable = true;
		BoardSourceDescriptor boardSource;
		std::uint64_t worldSeed = 0;
		std::uint64_t encounterOrdinal = 0;
		std::uint64_t encounterResolutionSeed = 0;
		std::uint64_t combatSeed = 0;
		std::uint32_t resolvedTier = 0;
		std::string resolvedTeamId;
		spk::Vector3Int returnWorldCell{};
	};

	// One event before it is committed: the header's battleId/sequence/batchId/time/turn/action are
	// stamped at commit, the category/origin/payload are supplied by the resolver.
	struct StagedEvent
	{
		BattleEventCategory category = BattleEventCategory::Gameplay;
		BattleEventOrigin origin;
		BattleEventPayload payload;
	};

	// Owns every mutable value of one battle: the board, the units, the RNG, the phase/outcome, the
	// checked runtime counters and the append-only log. It is never a global, never lives in
	// GameContext and never enters spk::Component. It is a private member of BattleSession, so the
	// public methods below - both the const queries and the mutation/transaction helpers - are only
	// ever reachable through the session's single command path; nothing outside can hold one.
	class BattleContext
	{
	public:
		// A command owns this value while it resolves.  Staged events have not reached the log yet,
		// so restoring it is enough to discard every mutation a numeric failure made before the
		// TechnicalAbort tail is committed.
		struct CommandRollbackState
		{
			BoardData board;
			std::map<BattleUnitId, BattleUnit> units;
			BattleRng rng{0};
			BattlePhase phase = BattlePhase::Deployment;
			BattleOutcome outcome = BattleOutcome::Undecided;
			std::optional<BattleAbortReason> abortReason;
			std::optional<BattleTerminalRecord> terminalRecord;
			std::optional<BattleUnitId> activeUnit;
			std::optional<TurnIndex> turn;
			std::uint64_t nextTurn = 1;
			std::size_t resolvedNonEndCommands = 0;
			BattleTime elapsed{};
			bool playerConfirmed = false;
			bool enemyConfirmed = false;
			BattleShieldIdAllocator shieldAllocator;
			BattleObjectIdAllocator objectAllocator;
			std::uint64_t nextAction = 1;
			std::uint64_t nextBatch = 1;
			std::uint64_t nextEvent = 1;
			bool ordinaryAllocationExhausted = false;
		};

	private:
		BattleDescriptor _descriptor;
		const Registries *_registries = nullptr;
		BoardData _board;
		std::map<BattleUnitId, BattleUnit> _units;
		std::vector<BattleUnitId> _playerOrder;
		std::vector<BattleUnitId> _enemyOrder;
		BattleRng _rng;
		BattlePhase _phase = BattlePhase::Deployment;
		BattleOutcome _outcome = BattleOutcome::Undecided;
		std::optional<BattleAbortReason> _abortReason;
		std::optional<BattleTerminalRecord> _terminalRecord;
		std::optional<BattleUnitId> _activeUnit;
		std::optional<TurnIndex> _turn; // step 07 assigns the first non-zero value
		std::uint64_t _nextTurn = 1;
		std::size_t _resolvedNonEndCommands = 0;
		BattleTime _elapsed{};

		bool _playerConfirmed = false;
		bool _enemyConfirmed = false;

		BattleEventLog _events;
		BattleShieldIdAllocator _shieldAllocator;
		BattleObjectIdAllocator _objectAllocator; // step 10 places the first object

		// Checked runtime counters. 0 is the unallocated sentinel; each starts at 1.
		std::uint64_t _nextAction = 1;
		std::uint64_t _nextBatch = 1;
		std::uint64_t _nextEvent = 1;
		bool _ordinaryAllocationExhausted = false;

		// Two distinct structures (section 13): the authoritative archive is retained through result
		// finalization; the publish queue is drained by takeCommittedBatches without touching it.
		std::vector<CommittedBattleBatch> _archive;
		std::vector<CommittedBattleBatch> _publishQueue;

		[[nodiscard]] CommittedBattleBatch _commit(
			BattleBatchKind p_kind,
			bool p_allocateAction,
			BattleSnapshot p_before,
			const std::vector<StagedEvent> &p_staged);

	public:
		BattleContext(
			BattleDescriptor p_descriptor,
			const Registries &p_registries,
			BoardData p_board,
			std::map<BattleUnitId, BattleUnit> p_units,
			std::vector<BattleUnitId> p_playerOrder,
			std::vector<BattleUnitId> p_enemyOrder,
			BattleRng p_rng);

		BattleContext(const BattleContext &) = delete;
		BattleContext &operator=(const BattleContext &) = delete;

		// ---- Queries -----------------------------------------------------------------------------
		[[nodiscard]] const BattleDescriptor &descriptor() const noexcept
		{
			return _descriptor;
		}
		[[nodiscard]] BattleId battleId() const noexcept
		{
			return _descriptor.battleId;
		}
		[[nodiscard]] const Registries &registries() const noexcept
		{
			return *_registries;
		}
		[[nodiscard]] const BoardData &board() const noexcept
		{
			return _board;
		}
		[[nodiscard]] BattlePhase phase() const noexcept
		{
			return _phase;
		}
		[[nodiscard]] BattleOutcome outcome() const noexcept
		{
			return _outcome;
		}
		[[nodiscard]] const std::optional<BattleAbortReason> &abortReason() const noexcept
		{
			return _abortReason;
		}
		[[nodiscard]] const std::optional<BattleTerminalRecord> &terminalRecord() const noexcept
		{
			return _terminalRecord;
		}
		[[nodiscard]] const std::optional<BattleUnitId> &activeUnit() const noexcept
		{
			return _activeUnit;
		}
		[[nodiscard]] const std::optional<TurnIndex> &turn() const noexcept
		{
			return _turn;
		}
		[[nodiscard]] BattleTime elapsed() const noexcept
		{
			return _elapsed;
		}
		[[nodiscard]] std::size_t resolvedNonEndCommands() const noexcept
		{
			return _resolvedNonEndCommands;
		}
		[[nodiscard]] bool isTerminal() const noexcept
		{
			return _phase == BattlePhase::Terminal;
		}

		[[nodiscard]] const std::vector<BattleUnitId> &playerOrder() const noexcept
		{
			return _playerOrder;
		}
		[[nodiscard]] const std::vector<BattleUnitId> &enemyOrder() const noexcept
		{
			return _enemyOrder;
		}
		[[nodiscard]] const std::vector<BattleUnitId> &sideOrder(BattleSide p_side) const noexcept;
		// Player roster order then Enemy roster order.
		[[nodiscard]] std::vector<BattleUnitId> allUnitsInOrder() const;

		[[nodiscard]] const BattleUnit *tryUnit(BattleUnitId p_id) const noexcept;
		[[nodiscard]] const BattleUnit &unit(BattleUnitId p_id) const;
		[[nodiscard]] bool sideConfirmed(BattleSide p_side) const noexcept;

		[[nodiscard]] std::size_t rngDrawCount() const noexcept
		{
			return _rng.drawCount();
		}
		[[nodiscard]] const BattleEventLog &events() const noexcept
		{
			return _events;
		}
		[[nodiscard]] const std::vector<CommittedBattleBatch> &archivedBatches() const noexcept
		{
			return _archive;
		}

		[[nodiscard]] BattleSnapshot snapshot() const;
		[[nodiscard]] CommandRollbackState captureCommandRollbackState() const;
		void restoreCommandRollbackState(CommandRollbackState p_state);

		// active / not-defeated counts for a BattleEnded payload.
		[[nodiscard]] int activeCount(BattleSide p_side) const noexcept;
		[[nodiscard]] int notDefeatedCount(BattleSide p_side) const noexcept;
		// Step 10 supplies timeline status/shield/object deadlines. The empty runtime deliberately
		// has no boundary rather than a polling interval.
		[[nodiscard]] std::optional<BattleTime> timeUntilNextTimelineBoundary() const noexcept
		{
			return std::nullopt;
		}
		void advanceTimeline(BattleTime, std::vector<StagedEvent> &) noexcept
		{
		}

		// ---- Mutation surface (session / resolver only) ------------------------------------------
		[[nodiscard]] BattleRng &rng() noexcept
		{
			return _rng;
		}
		[[nodiscard]] BattleUnit &unitMutable(BattleUnitId p_id);

		// Occupancy edits paired with the unit's placement flags. Return false if the board rejects
		// the edit (the resolver validated first, so a false here is an internal invariant failure).
		[[nodiscard]] bool placeUnitOccupancy(BattleUnitId p_unit, const BoardCell &p_cell);
		[[nodiscard]] bool moveUnitOccupancy(BattleUnitId p_unit, const BoardCell &p_cell);
		[[nodiscard]] bool swapUnitsOccupancy(BattleUnitId p_first, BattleUnitId p_second);
		void removeUnitOccupancy(BattleUnitId p_unit);

		void setSideConfirmed(BattleSide p_side, bool p_confirmed) noexcept;
		void setPhase(BattlePhase p_phase) noexcept
		{
			_phase = p_phase;
		}
		void setOutcome(BattleOutcome p_outcome) noexcept
		{
			_outcome = p_outcome;
		}
		void setTerminal(BattleOutcome p_outcome, std::optional<BattleAbortReason> p_reason = std::nullopt);
		void setElapsed(BattleTime p_elapsed) noexcept
		{
			_elapsed = p_elapsed;
		}
		void setActiveUnit(std::optional<BattleUnitId> p_unit) noexcept
		{
			_activeUnit = p_unit;
		}
		void setTurn(std::optional<TurnIndex> p_turn) noexcept
		{
			_turn = p_turn;
		}
		[[nodiscard]] std::optional<TurnIndex> allocateTurn();
		[[nodiscard]] bool canAllocateTurn() const noexcept;
		void setResolvedNonEndCommands(std::size_t p_count) noexcept
		{
			_resolvedNonEndCommands = p_count;
		}
		[[nodiscard]] BattleObjectId allocateObjectId()
		{
			return _objectAllocator.allocate();
		}
		[[nodiscard]] BattleShieldId allocateShieldId()
		{
			return _shieldAllocator.allocate();
		}

		// Internal removal primitive (section 16). Emits UnitDefeated (only for Defeated) then
		// UnitRemoved as staged events appended to p_staged; occupancy is cleared exactly once. Step
		// 09 calls it after a pending HP-zero transition; step 16 through the taming service. It does
		// not itself commit a batch: the enclosing transaction owns the boundary.
		void removeUnit(BattleUnitId p_unit, RemovalReason p_reason, const BattleEventOrigin &p_origin, std::vector<StagedEvent> &p_staged);

		// ---- Transaction -------------------------------------------------------------------------
		// True when an ordinary batch of p_eventCount events (plus an action id when p_needsAction)
		// can commit while still leaving the terminal-abort reserve: >= 1 unused batch id and >= 3
		// contiguous event ids, and without any counter allocating its numeric maximum.
		[[nodiscard]] bool hasOrdinaryCapacity(std::size_t p_eventCount, bool p_needsAction) const noexcept;

		// Commits one non-empty ordinary batch (Construction/Command/Timeline/TamingSystem). Captures
		// the after snapshot itself. The caller must have confirmed hasOrdinaryCapacity first.
		[[nodiscard]] CommittedBattleBatch commitOrdinaryBatch(
			BattleBatchKind p_kind,
			BattleSnapshot p_before,
			const std::vector<StagedEvent> &p_staged);

		// Discards nothing of its own (the caller already dropped its scratch): commits the reserved
		// no-action TechnicalAbort tail (BattleAborted then BattleEnded{Aborted}) from p_before, marks
		// the battle terminal and marks ordinary allocation exhausted.
		[[nodiscard]] CommittedBattleBatch commitAbortBatch(BattleAbortReason p_reason, BattleSnapshot p_before);

		// Drains the publication queue in commit order; never touches the authoritative archive.
		[[nodiscard]] std::vector<CommittedBattleBatch> takeCommittedBatches();

		// Test seam (section 18.5): drive the checked counters near their maximum so a following
		// ordinary transaction must roll back into the reserved TechnicalAbort batch. Not production.
		void debugSetNextCounters(std::uint64_t p_nextAction, std::uint64_t p_nextBatch, std::uint64_t p_nextEvent) noexcept;
		void debugSetNextShieldIdForExhaustionTest(std::uint32_t p_next) noexcept;
	};
}
