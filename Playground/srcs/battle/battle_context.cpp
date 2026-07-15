#include "battle/battle_context.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace pg
{
	namespace
	{
		constexpr std::uint64_t CounterMax = std::numeric_limits<std::uint64_t>::max();
		// The terminal-abort tail reserve: >= 1 unused batch id and >= 3 contiguous event ids.
		constexpr std::uint64_t ReservedBatches = 1;
		constexpr std::uint64_t ReservedEvents = 3;
	}

	BattleContext::BattleContext(
		BattleDescriptor p_descriptor,
		const Registries &p_registries,
		BoardData p_board,
		std::map<BattleUnitId, BattleUnit> p_units,
		std::vector<BattleUnitId> p_playerOrder,
		std::vector<BattleUnitId> p_enemyOrder,
		BattleRng p_rng) :
		_descriptor(std::move(p_descriptor)),
		_registries(&p_registries),
		_board(std::move(p_board)),
		_units(std::move(p_units)),
		_playerOrder(std::move(p_playerOrder)),
		_enemyOrder(std::move(p_enemyOrder)),
		_rng(std::move(p_rng))
	{
	}

	const std::vector<BattleUnitId> &BattleContext::sideOrder(BattleSide p_side) const noexcept
	{
		return p_side == BattleSide::Player ? _playerOrder : _enemyOrder;
	}

	std::vector<BattleUnitId> BattleContext::allUnitsInOrder() const
	{
		std::vector<BattleUnitId> ordered;
		ordered.reserve(_playerOrder.size() + _enemyOrder.size());
		ordered.insert(ordered.end(), _playerOrder.begin(), _playerOrder.end());
		ordered.insert(ordered.end(), _enemyOrder.begin(), _enemyOrder.end());
		return ordered;
	}

	const BattleUnit *BattleContext::tryUnit(BattleUnitId p_id) const noexcept
	{
		const auto found = _units.find(p_id);
		return found == _units.end() ? nullptr : &found->second;
	}

	const BattleUnit &BattleContext::unit(BattleUnitId p_id) const
	{
		const BattleUnit *found = tryUnit(p_id);
		if (found == nullptr)
		{
			throw std::out_of_range("no battle unit with id " + std::to_string(p_id.value()));
		}
		return *found;
	}

	BattleUnit &BattleContext::unitMutable(BattleUnitId p_id)
	{
		const auto found = _units.find(p_id);
		if (found == _units.end())
		{
			throw std::out_of_range("no battle unit with id " + std::to_string(p_id.value()));
		}
		return found->second;
	}

	bool BattleContext::sideConfirmed(BattleSide p_side) const noexcept
	{
		return p_side == BattleSide::Player ? _playerConfirmed : _enemyConfirmed;
	}

	void BattleContext::setSideConfirmed(BattleSide p_side, bool p_confirmed) noexcept
	{
		(p_side == BattleSide::Player ? _playerConfirmed : _enemyConfirmed) = p_confirmed;
	}

	int BattleContext::activeCount(BattleSide p_side) const noexcept
	{
		int count = 0;
		for (const BattleUnitId id : sideOrder(p_side))
		{
			if (unit(id).isActiveCombatant())
			{
				++count;
			}
		}
		return count;
	}

	int BattleContext::notDefeatedCount(BattleSide p_side) const noexcept
	{
		int count = 0;
		for (const BattleUnitId id : sideOrder(p_side))
		{
			if (unit(id).removalReason() != RemovalReason::Defeated)
			{
				++count;
			}
		}
		return count;
	}

	bool BattleContext::placeUnitOccupancy(BattleUnitId p_unit, const BoardCell &p_cell)
	{
		const BoardMutationResult result = BoardMutation(_board).placeUnit(p_unit, p_cell);
		if (!result.accepted)
		{
			return false;
		}
		unitMutable(p_unit).markPlacedAt(p_cell);
		return true;
	}

	bool BattleContext::moveUnitOccupancy(BattleUnitId p_unit, const BoardCell &p_cell)
	{
		const BoardMutationResult result = BoardMutation(_board).moveUnit(p_unit, p_cell);
		if (!result.accepted)
		{
			return false;
		}
		unitMutable(p_unit).markPlacedAt(p_cell);
		return true;
	}

	bool BattleContext::swapUnitsOccupancy(BattleUnitId p_first, BattleUnitId p_second)
	{
		const BoardMutationResult result = BoardMutation(_board).swapUnits(p_first, p_second);
		if (!result.accepted)
		{
			return false;
		}
		// After the swap each unit stands on the other's former cell; mirror it into the flags.
		const std::optional<BoardCell> firstCell = _board.occupancy().cellOf(p_first);
		const std::optional<BoardCell> secondCell = _board.occupancy().cellOf(p_second);
		if (firstCell.has_value())
		{
			unitMutable(p_first).markPlacedAt(*firstCell);
		}
		if (secondCell.has_value())
		{
			unitMutable(p_second).markPlacedAt(*secondCell);
		}
		return true;
	}

	void BattleContext::removeUnitOccupancy(BattleUnitId p_unit)
	{
		(void)BoardMutation(_board).removeUnit(p_unit);
		unitMutable(p_unit).clearPlacement();
	}

	void BattleContext::removeUnit(
		BattleUnitId p_unit,
		RemovalReason p_reason,
		const BattleEventOrigin &p_origin,
		std::vector<StagedEvent> &p_staged)
	{
		BattleUnit &target = unitMutable(p_unit);
		if (p_reason == RemovalReason::None)
		{
			throw std::invalid_argument("removeUnit needs a terminal reason, not None");
		}
		// Idempotent: the same terminal reason already applied is a no-op, so a double HP-zero or a
		// second taming attempt cannot append a second removal sequence.
		if (target.removalReason() == p_reason)
		{
			return;
		}

		const std::optional<BoardCell> previousCell = _board.occupancy().cellOf(p_unit);
		const BoardCell factCell = previousCell.value_or(target.lastOccupiedCell().value_or(BoardCell{}));

		// Set the reason and clear occupancy exactly once BEFORE emitting the removal facts.
		target.setRemovalReason(p_reason);
		removeUnitOccupancy(p_unit);

		// UnitDefeated first, only for Defeated. Impressed is not defeat and never gets kill credit.
		if (p_reason == RemovalReason::Defeated)
		{
			UnitDefeated defeated;
			defeated.unit = p_unit;
			defeated.killer = p_origin.sourceUnit;
			defeated.abilityId = p_origin.abilityId;
			defeated.effectId = p_origin.effectId;
			defeated.previousCell = factCell;
			p_staged.push_back(StagedEvent{BattleEventCategory::Gameplay, p_origin, defeated});
		}

		// Step 10 owner-cleanup (statuses, shields, owner-duration objects) belongs between the two
		// facts, with every removal hook suppressed. There are no cleanup instances before step 10,
		// so the two facts are adjacent here.

		UnitRemoved removed;
		removed.unit = p_unit;
		removed.previousCell = previousCell.has_value() ? std::optional<BoardCell>(factCell) : target.lastOccupiedCell();
		removed.reason = p_reason;
		p_staged.push_back(StagedEvent{BattleEventCategory::Gameplay, p_origin, removed});
	}

	BattleSnapshot BattleContext::snapshot() const
	{
		BattleSnapshot snap;
		snap.battleId = _descriptor.battleId;
		snap.boardSource = _board.sourceDescriptor();
		snap.phase = _phase;
		snap.outcome = _outcome;
		snap.abortReason = _abortReason;
		snap.elapsed = _elapsed;
		snap.turn = _turn;
		snap.activeUnit = _activeUnit;

		const auto capture = [&](BattleUnitId p_id) {
			const BattleUnit &u = unit(p_id);
			BattleUnitSnapshot us;
			us.id = u.id();
			us.side = u.side();
			us.rosterOrder = u.rosterOrder();
			us.persistentCreatureId = u.persistentCreatureId();
			us.encounterSpawnMemberId = u.encounterSpawnMemberId();
			us.inheritedCompletedFeatNodeIds = u.inheritedCompletedFeatNodeIds();
			us.speciesId = u.speciesId();
			us.formId = u.formId();
			us.health = u.health();
			us.maxHealth = u.maxHealth();
			us.actionPoints = u.actionPoints();
			us.maxActionPoints = u.maxActionPoints();
			us.movementPoints = u.movementPoints();
			us.maxMovementPoints = u.maxMovementPoints();
			us.stamina = u.effectiveAttributes().stamina;
			us.turnBarFill = u.turnBarFill();
			us.range = u.range();
			us.placed = u.placed();
			us.cell = _board.occupancy().cellOf(p_id);
			us.lastOccupiedCell = u.lastOccupiedCell();
			us.removalReason = u.removalReason();
			snap.units.push_back(std::move(us));
		};

		for (const BattleUnitId id : _playerOrder)
		{
			capture(id);
		}
		for (const BattleUnitId id : _enemyOrder)
		{
			capture(id);
		}
		return snap;
	}

	bool BattleContext::hasOrdinaryCapacity(std::size_t p_eventCount, bool p_needsAction) const noexcept
	{
		if (_ordinaryAllocationExhausted)
		{
			return false;
		}
		const std::uint64_t events = static_cast<std::uint64_t>(p_eventCount);

		// Batch: this batch plus the reserved one both fit (never allocate the numeric maximum for an
		// ordinary batch, and leave one batch id for the abort tail).
		if (_nextBatch > CounterMax - ReservedBatches)
		{
			return false;
		}
		// Events: p_eventCount now plus the 3-id abort tail, all <= the maximum.
		if (_nextEvent > CounterMax - events - ReservedEvents)
		{
			return false;
		}
		// Action: an ordinary command never allocates the numeric maximum action id.
		if (p_needsAction && _nextAction >= CounterMax)
		{
			return false;
		}
		return true;
	}

	CommittedBattleBatch BattleContext::_commit(
		BattleBatchKind p_kind,
		bool p_allocateAction,
		BattleSnapshot p_before,
		const std::vector<StagedEvent> &p_staged)
	{
		if (p_staged.empty())
		{
			throw std::logic_error("no empty battle batch is ever committed");
		}

		CommittedBattleBatch batch;
		batch.id = BattleBatchId{_nextBatch++};
		batch.kind = p_kind;

		std::optional<BattleActionId> action;
		if (p_allocateAction)
		{
			action = BattleActionId{_nextAction++};
		}
		batch.action = action;

		const BattleEventSequence first{_nextEvent};
		for (const StagedEvent &staged : p_staged)
		{
			BattleEvent event;
			event.header.battleId = _descriptor.battleId;
			event.header.sequence = BattleEventSequence{_nextEvent++};
			event.header.batchId = batch.id;
			event.header.battleTime = _elapsed;
			event.header.turn = _turn;
			event.header.action = action;
			event.header.category = staged.category;
			event.header.origin = staged.origin;
			event.payload = staged.payload;
			_events.append(std::move(event));
		}
		batch.events = EventRange{first, BattleEventSequence{_nextEvent}};

		batch.before = std::move(p_before);
		batch.after = snapshot();

		_archive.push_back(batch);
		_publishQueue.push_back(batch);
		return batch;
	}

	CommittedBattleBatch BattleContext::commitOrdinaryBatch(
		BattleBatchKind p_kind,
		BattleSnapshot p_before,
		const std::vector<StagedEvent> &p_staged)
	{
		return _commit(p_kind, p_kind == BattleBatchKind::Command, std::move(p_before), p_staged);
	}

	CommittedBattleBatch BattleContext::commitAbortBatch(BattleAbortReason p_reason, BattleSnapshot p_before)
	{
		// The reserve must still be present; the only way it is not is memory corruption or a
		// programmer violation, which we fail fast on rather than emit a malformed log.
		if (_nextBatch > CounterMax - ReservedBatches || _nextEvent > CounterMax - ReservedEvents)
		{
			throw std::logic_error("terminal-abort reserve was already consumed: battle state is corrupt");
		}

		_phase = BattlePhase::Terminal;
		_outcome = BattleOutcome::Aborted;
		_abortReason = p_reason;
		_activeUnit = std::nullopt;

		std::vector<StagedEvent> staged;
		staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, BattleAborted{p_reason}});

		BattleEnded ended;
		ended.outcome = BattleOutcome::Aborted;
		ended.activePlayerCount = activeCount(BattleSide::Player);
		ended.activeEnemyCount = activeCount(BattleSide::Enemy);
		ended.notDefeatedPlayerCount = notDefeatedCount(BattleSide::Player);
		ended.notDefeatedEnemyCount = notDefeatedCount(BattleSide::Enemy);
		staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, ended});

		CommittedBattleBatch batch = _commit(BattleBatchKind::TechnicalAbort, false, std::move(p_before), staged);
		_ordinaryAllocationExhausted = true;
		return batch;
	}

	std::vector<CommittedBattleBatch> BattleContext::takeCommittedBatches()
	{
		std::vector<CommittedBattleBatch> drained = std::move(_publishQueue);
		_publishQueue.clear();
		return drained;
	}

	void BattleContext::debugSetNextCounters(
		std::uint64_t p_nextAction,
		std::uint64_t p_nextBatch,
		std::uint64_t p_nextEvent) noexcept
	{
		_nextAction = p_nextAction;
		_nextBatch = p_nextBatch;
		_nextEvent = p_nextEvent;
	}
}
