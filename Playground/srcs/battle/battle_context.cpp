#include "battle/battle_context.hpp"

#include "battle/status/status_hook_service.hpp"
#include "core/registries.hpp"
#include "statuses/status_definition.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
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

	bool BattleContext::hasStatus(BattleUnitId p_id, std::string_view p_statusId) const
	{
		const BattleUnit &target = unit(p_id);
		const auto contains = [p_statusId](const std::vector<BattleStatusInstance> &statuses) {
			return std::ranges::any_of(statuses, [p_statusId](const BattleStatusInstance &status) {
				return status.definitionId == p_statusId;
			});
		};
		return contains(target.passiveStatuses()) || contains(target.transientStatuses());
	}

	bool BattleContext::hasStatusTag(BattleUnitId p_id, std::string_view p_tag) const
	{
		const BattleUnit &target = unit(p_id);
		const auto containsTag = [this, p_tag](const std::vector<BattleStatusInstance> &statuses) {
			for (const BattleStatusInstance &instance : statuses)
			{
				const StatusDefinition *definition = registries().statuses().tryGet(instance.definitionId);
				if (definition != nullptr && std::ranges::find(definition->tags, p_tag) != definition->tags.end())
				{
					return true;
				}
			}
			return false;
		};
		return containsTag(target.passiveStatuses()) || containsTag(target.transientStatuses());
	}

	bool BattleContext::isTurnBarPaused(BattleUnitId p_id) const
	{
		return unit(p_id).isStunned();
	}

	CreatureAttributes BattleContext::effectiveAttributes(BattleUnitId p_id) const
	{
		return unit(p_id).effectiveAttributes();
	}

	const BattleObjectInstance *BattleContext::tryObject(BattleObjectId p_id) const noexcept
	{
		const auto found = _objects.find(p_id);
		return found == _objects.end() ? nullptr : &found->second;
	}

	BattleObjectInstance *BattleContext::objectMutable(BattleObjectId p_id) noexcept
	{
		const auto found = _objects.find(p_id);
		return found == _objects.end() ? nullptr : &found->second;
	}

	bool BattleContext::blocksMovement(const BoardCell &p_cell) const
	{
		for (const BattleObjectId id : _board.occupancy().objectsAt(p_cell))
		{
			const BattleObjectInstance *instance = tryObject(id);
			const BattleObjectDefinition *definition = instance == nullptr ? nullptr : registries().battleObjects().tryGet(instance->definitionId);
			if (definition != nullptr && definition->blocksMovement)
			{
				return true;
			}
		}
		return false;
	}

	bool BattleContext::blocksLineOfSightVoxel(const spk::Vector3Int &p_voxel) const
	{
		const BoardCell support{p_voxel.x, p_voxel.y - 1, p_voxel.z};
		for (const BattleObjectId id : _board.occupancy().objectsAt(support))
		{
			const BattleObjectInstance *instance = tryObject(id);
			const BattleObjectDefinition *definition = instance == nullptr ? nullptr : registries().battleObjects().tryGet(instance->definitionId);
			if (definition != nullptr && definition->blocksLineOfSight)
			{
				return true;
			}
		}
		return false;
	}

	void BattleContext::projectPassiveStatuses()
	{
		for (const BattleUnitId id : allUnitsInOrder())
		{
			BattleUnit &target = unitMutable(id);
			for (const std::string &statusId : target.passiveStatusIds())
			{
				target.passiveStatusesMutable().push_back(BattleStatusInstance{.id = allocateStatusInstanceId(), .definitionId = statusId, .origin = BattleStatusOrigin::PassiveProjection, .stacks = 1, .duration = InfiniteDurationState{}, .appliedBy = id});
			}
			reconcileEffectiveStats(id);
		}
	}

	void BattleContext::reconcileEffectiveStats(BattleUnitId p_unit, std::vector<StagedEvent> *p_staged)
	{
		BattleUnit &target = unitMutable(p_unit);
		const CreatureAttributes previous = target.effectiveAttributes();
		const CreatureAttributes next = EffectiveStatsEvaluator::evaluate(target, registries());
		const auto emit = [p_staged, p_unit](CreatureStat p_stat, std::int64_t p_before, std::int64_t p_after) {
			if (p_staged != nullptr && p_before != p_after)
			{
				p_staged->push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = p_unit}, EffectiveStatChanged{p_unit, p_stat, p_before, p_after}});
			}
		};
		emit(CreatureStat::MaxHealth, previous.maxHealth, next.maxHealth);
		emit(CreatureStat::Strength, previous.strength, next.strength);
		emit(CreatureStat::MagicPower, previous.magicPower, next.magicPower);
		emit(CreatureStat::Armor, previous.armor, next.armor);
		emit(CreatureStat::Resistance, previous.resistance, next.resistance);
		emit(CreatureStat::MaxActionPoints, previous.maxActionPoints, next.maxActionPoints);
		emit(CreatureStat::MaxMovementPoints, previous.maxMovementPoints, next.maxMovementPoints);
		emit(CreatureStat::Stamina, previous.stamina.ticks(), next.stamina.ticks());
		emit(CreatureStat::Range, previous.range, next.range);
		target.setEffectiveAttributes(next);
		target.setTurnBarPaused(hasStatusTag(p_unit, StunTag));

		const auto clampPool = [p_staged, p_unit](BattleResource p_resource, int p_before, int p_after) {
			if (p_staged != nullptr && p_before != p_after)
			{
				p_staged->push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = p_unit}, ResourceChanged{p_unit, p_resource, p_after - p_before, p_after - p_before, p_before, p_after, ResourceChangeReason::EffectiveMaximumClamp}});
			}
		};
		if (target.health() > target.maxHealth())
		{
			target.setHealth(target.maxHealth());
		}
		if (target.actionPoints() > target.maxActionPoints())
		{
			const int before = target.actionPoints();
			target.setActionPoints(target.maxActionPoints());
			clampPool(BattleResource::ActionPoints, before, target.actionPoints());
		}
		if (target.movementPoints() > target.maxMovementPoints())
		{
			const int before = target.movementPoints();
			target.setMovementPoints(target.maxMovementPoints());
			clampPool(BattleResource::MovementPoints, before, target.movementPoints());
		}
		if (target.turnBarFill() > target.effectiveAttributes().stamina)
		{
			target.setTurnBarFill(target.effectiveAttributes().stamina);
		}
	}

	std::optional<BattleTime> BattleContext::timeUntilNextTimelineBoundary() const noexcept
	{
		std::optional<BattleTime> result;
		const auto consider = [this, &result](const DurationState &duration) {
			const auto *timeline = std::get_if<TimelineDurationState>(&duration);
			if (timeline == nullptr)
			{
				return;
			}
			const BattleTime delta = timeline->expiresAt > _elapsed ? timeline->expiresAt - _elapsed : BattleTime{};
			if (!result.has_value() || delta < *result)
			{
				result = delta;
			}
		};
		for (const BattleUnitId id : allUnitsInOrder())
		{
			const BattleUnit &battleUnit = unit(id);
			for (const BattleStatusInstance &status : battleUnit.transientStatuses())
			{
				consider(status.duration);
			}
			for (const BattleShield &shield : battleUnit.shields())
			{
				consider(shield.duration);
			}
		}
		for (const auto &[id, object] : _objects)
		{
			(void)id;
			consider(object.duration);
		}
		return result;
	}

	void BattleContext::advanceTimeline(BattleTime, std::vector<StagedEvent> &p_staged)
	{
		const auto expired = [this](const DurationState &duration) {
			const auto *timeline = std::get_if<TimelineDurationState>(&duration);
			return timeline != nullptr && timeline->expiresAt <= _elapsed;
		};
		// The outer order is participant order, then stable instance allocation order.  Erasing in
		// place is safe because a transient's position is not its identity.
		for (const BattleUnitId id : allUnitsInOrder())
		{
			BattleUnit &target = unitMutable(id);
			while (true)
			{
				auto &statuses = target.transientStatusesMutable();
				auto it = std::ranges::find_if(statuses, [&expired](const BattleStatusInstance &status) {
					return expired(status.duration);
				});
				if (it == statuses.end())
				{
					break;
				}
				const BattleStatusInstance removed = *it;
				const StatusDefinition *definition = registries().statuses().tryGet(removed.definitionId);
				statuses.erase(it);
				reconcileEffectiveStats(id, &p_staged);
				StatusRemoved event;
				event.originalApplier = removed.appliedBy;
				event.target = id;
				event.statusId = removed.definitionId;
				event.requestedStacks = static_cast<int>(removed.stacks);
				event.removedStacks = static_cast<int>(removed.stacks);
				event.reason = StatusRemovalReason::TimelineExpired;
				event.tags = definition == nullptr ? std::vector<std::string>{} : definition->tags;
				event.instanceId = removed.id;
				p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = id, .statusId = removed.definitionId}, std::move(event)});
				StatusHookService::dispatchRemovedHook(*this, p_staged, id, removed);
			}
			for (auto it = target.shieldsMutable().begin(); it != target.shieldsMutable().end();)
			{
				if (!expired(it->duration))
				{
					++it;
					continue;
				}
				const BattleShield removed = *it;
				it = target.shieldsMutable().erase(it);
				p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = id}, ShieldRemoved{id, removed.id, removed.kind, ShieldRemovalReason::TimelineExpired, static_cast<int>(removed.remainingAmount), removed.source, removed.sourceAbilityId, removed.sourceEffectId}});
			}
		}
		std::vector<BattleObjectId> expiredObjects;
		for (const auto &[id, object] : _objects)
		{
			if (expired(object.duration))
			{
				expiredObjects.push_back(id);
			}
		}
		for (const BattleObjectId id : expiredObjects)
		{
			const BattleObjectInstance copy = _objects.find(id)->second;
			(void)removeObject(id);
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = copy.creator, .objectId = id}, BattleObjectRemoved{id, copy.definitionId, copy.cell, BattleObjectRemovalReason::TimelineExpired}});
		}
	}

	void BattleContext::captureOwnerActivationDurations(BattleUnitId p_owner)
	{
		_capturedStatusDurations.clear();
		_capturedShieldDurations.clear();
		_capturedObjectDurations.clear();
		const BattleUnit &owner = unit(p_owner);
		for (const BattleStatusInstance &status : owner.transientStatuses())
		{
			if (std::holds_alternative<OwnerActivationDurationState>(status.duration))
			{
				_capturedStatusDurations.push_back(status.id);
			}
		}
		for (const BattleShield &shield : owner.shields())
		{
			if (std::holds_alternative<OwnerActivationDurationState>(shield.duration))
			{
				_capturedShieldDurations.push_back(shield.id);
			}
		}
		for (const auto &[id, object] : _objects)
		{
			if (object.creator == std::optional<BattleUnitId>(p_owner) && std::holds_alternative<OwnerActivationDurationState>(object.duration))
			{
				_capturedObjectDurations.push_back(id);
			}
		}
	}

	void BattleContext::expireCapturedOwnerActivationDurations(BattleUnitId p_owner, std::vector<StagedEvent> &p_staged)
	{
		BattleUnit &owner = unitMutable(p_owner);
		for (const BattleStatusInstanceId id : _capturedStatusDurations)
		{
			auto &statuses = owner.transientStatusesMutable();
			auto found = std::ranges::find_if(statuses, [id](const BattleStatusInstance &status) {
				return status.id == id;
			});
			if (found == statuses.end())
			{
				continue;
			}
			auto *duration = std::get_if<OwnerActivationDurationState>(&found->duration);
			if (duration == nullptr || duration->remainingActivations == 0)
			{
				continue;
			}
			--duration->remainingActivations;
			if (duration->remainingActivations != 0)
			{
				continue;
			}
			const BattleStatusInstance removed = *found;
			const StatusDefinition *definition = registries().statuses().tryGet(removed.definitionId);
			statuses.erase(found);
			reconcileEffectiveStats(p_owner, &p_staged);
			StatusRemoved event;
			event.originalApplier = removed.appliedBy;
			event.target = p_owner;
			event.statusId = removed.definitionId;
			event.requestedStacks = static_cast<int>(removed.stacks);
			event.removedStacks = static_cast<int>(removed.stacks);
			event.reason = StatusRemovalReason::OwnerActivationsExpired;
			event.tags = definition == nullptr ? std::vector<std::string>{} : definition->tags;
			event.instanceId = removed.id;
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = p_owner, .statusId = removed.definitionId}, std::move(event)});
			StatusHookService::dispatchRemovedHook(*this, p_staged, p_owner, removed);
		}
		for (const BattleShieldId id : _capturedShieldDurations)
		{
			auto &shields = owner.shieldsMutable();
			auto found = std::ranges::find_if(shields, [id](const BattleShield &shield) {
				return shield.id == id;
			});
			if (found == shields.end())
			{
				continue;
			}
			auto *duration = std::get_if<OwnerActivationDurationState>(&found->duration);
			if (duration == nullptr || duration->remainingActivations == 0)
			{
				continue;
			}
			--duration->remainingActivations;
			if (duration->remainingActivations != 0)
			{
				continue;
			}
			const BattleShield removed = *found;
			shields.erase(found);
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = p_owner}, ShieldRemoved{p_owner, removed.id, removed.kind, ShieldRemovalReason::OwnerActivationsExpired, static_cast<int>(removed.remainingAmount), removed.source, removed.sourceAbilityId, removed.sourceEffectId}});
		}
		for (const BattleObjectId id : _capturedObjectDurations)
		{
			BattleObjectInstance *object = objectMutable(id);
			if (object == nullptr)
			{
				continue;
			}
			auto *duration = std::get_if<OwnerActivationDurationState>(&object->duration);
			if (duration == nullptr || duration->remainingActivations == 0)
			{
				continue;
			}
			--duration->remainingActivations;
			if (duration->remainingActivations != 0)
			{
				continue;
			}
			const BattleObjectInstance copy = *object;
			(void)removeObject(id);
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = copy.creator, .objectId = id}, BattleObjectRemoved{id, copy.definitionId, copy.cell, BattleObjectRemovalReason::OwnerActivationsExpired}});
		}
		_capturedStatusDurations.clear();
		_capturedShieldDurations.clear();
		_capturedObjectDurations.clear();
	}

	bool BattleContext::placeObject(BattleObjectInstance p_object)
	{
		if (_objects.contains(p_object.id) || !BoardMutation(_board).placeObject(p_object.id, p_object.cell).accepted)
		{
			return false;
		}
		_objects.emplace(p_object.id, std::move(p_object));
		return true;
	}

	bool BattleContext::removeObject(BattleObjectId p_object)
	{
		if (!_objects.contains(p_object))
		{
			return false;
		}
		(void)BoardMutation(_board).removeObject(p_object);
		_objects.erase(p_object);
		return true;
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

	void BattleContext::setTerminal(BattleOutcome p_outcome, std::optional<BattleAbortReason> p_reason)
	{
		if (_terminalRecord.has_value())
		{
			throw std::logic_error("battle terminal record may only be constructed once");
		}
		if ((p_outcome == BattleOutcome::Aborted) != p_reason.has_value())
		{
			throw std::invalid_argument("an abort reason is present exactly for an aborted terminal outcome");
		}
		_outcome = p_outcome;
		_abortReason = p_reason;
		_phase = BattlePhase::Terminal;
		_terminalRecord = BattleTerminalRecord{_descriptor.battleId, p_outcome, p_reason};
	}

	std::optional<TurnIndex> BattleContext::allocateTurn()
	{
		if (_nextTurn >= CounterMax)
		{
			return std::nullopt;
		}
		return TurnIndex{_nextTurn++};
	}

	bool BattleContext::canAllocateTurn() const noexcept
	{
		return _nextTurn < CounterMax;
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

		// Owner cleanup is intentionally hook-suppressed: a defeated creature cannot turn a cleanse
		// or a disappearing shield into a second post-defeat action.
		for (const BattleStatusInstance &status : target.transientStatuses())
		{
			const StatusDefinition *definition = registries().statuses().tryGet(status.definitionId);
			StatusRemoved statusRemoved;
			statusRemoved.originalApplier = status.appliedBy;
			statusRemoved.target = p_unit;
			statusRemoved.statusId = status.definitionId;
			statusRemoved.requestedStacks = static_cast<int>(status.stacks);
			statusRemoved.removedStacks = static_cast<int>(status.stacks);
			statusRemoved.reason = StatusRemovalReason::UnitRemoved;
			statusRemoved.tags = definition == nullptr ? std::vector<std::string>{} : definition->tags;
			statusRemoved.instanceId = status.id;
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = p_unit, .statusId = status.definitionId}, std::move(statusRemoved)});
		}
		target.transientStatusesMutable().clear();
		for (const BattleShield &shield : target.shields())
		{
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.targetUnit = p_unit}, ShieldRemoved{p_unit, shield.id, shield.kind, ShieldRemovalReason::OwnerRemoved, static_cast<int>(shield.remainingAmount), shield.source, shield.sourceAbilityId, shield.sourceEffectId}});
		}
		target.shieldsMutable().clear();
		std::vector<BattleObjectId> owned;
		for (const auto &[id, object] : _objects)
		{
			if (object.creator == std::optional<BattleUnitId>(p_unit) && std::holds_alternative<OwnerActivationDurationState>(object.duration))
			{
				owned.push_back(id);
			}
		}
		for (const BattleObjectId id : owned)
		{
			const BattleObjectInstance copy = _objects.find(id)->second;
			(void)removeObject(id);
			p_staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = p_unit, .objectId = id}, BattleObjectRemoved{id, copy.definitionId, copy.cell, BattleObjectRemovalReason::OwnerRemoved}});
		}
		if (_activeUnit == p_unit)
		{
			_capturedStatusDurations.clear();
			_capturedShieldDurations.clear();
			_capturedObjectDurations.clear();
		}

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
		snap.nextTurn = _nextTurn;
		snap.activeUnit = _activeUnit;
		snap.resolvedNonEndCommands = _resolvedNonEndCommands;

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
			us.baselineAttributes = u.baselineAttributes();
			us.effectiveAttributes = u.effectiveAttributes();
			us.placed = u.placed();
			us.cell = _board.occupancy().cellOf(p_id);
			us.lastOccupiedCell = u.lastOccupiedCell();
			us.removalReason = u.removalReason();
			for (const BattleShield &shield : u.shields())
			{
				us.shields.push_back(BattleShieldSnapshot{shield.id, shield.kind, shield.remainingAmount, snapshotDuration(shield.duration), shield.source, shield.sourceAbilityId, shield.sourceEffectId});
			}
			for (const BattleStatusInstance &status : u.passiveStatuses())
			{
				us.passiveStatuses.push_back(snapshotStatus(status));
			}
			for (const BattleStatusInstance &status : u.transientStatuses())
			{
				us.transientStatuses.push_back(snapshotStatus(status));
			}
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
		for (const auto &[id, object] : _objects)
		{
			const BattleObjectDefinition *definition = registries().battleObjects().tryGet(object.definitionId);
			snap.objects.push_back(BattleObjectSnapshot{.id = id, .definitionId = object.definitionId, .creator = object.creator, .creatorSide = object.creatorSide, .cell = object.cell, .duration = snapshotDuration(object.duration), .triggerStates = object.triggerStates, .blocksMovement = definition != nullptr && definition->blocksMovement, .blocksLineOfSight = definition != nullptr && definition->blocksLineOfSight});
		}
		return snap;
	}

	BattleContext::CommandRollbackState BattleContext::captureCommandRollbackState() const
	{
		CommandRollbackState state{
			.board = _board,
			.units = _units,
			.rng = _rng,
			.phase = _phase,
			.outcome = _outcome,
			.abortReason = _abortReason,
			.terminalRecord = _terminalRecord,
			.activeUnit = _activeUnit,
			.turn = _turn,
			.nextTurn = _nextTurn,
			.resolvedNonEndCommands = _resolvedNonEndCommands,
			.elapsed = _elapsed,
			.playerConfirmed = _playerConfirmed,
			.enemyConfirmed = _enemyConfirmed,
			.shieldAllocator = _shieldAllocator,
			.objectAllocator = _objectAllocator,
			.statusAllocator = _statusAllocator,
			.objects = _objects,
			.capturedStatusDurations = _capturedStatusDurations,
			.capturedShieldDurations = _capturedShieldDurations,
			.capturedObjectDurations = _capturedObjectDurations,
			.nextAction = _nextAction,
			.nextBatch = _nextBatch,
			.nextEvent = _nextEvent,
			.ordinaryAllocationExhausted = _ordinaryAllocationExhausted};
		return state;
	}

	void BattleContext::restoreCommandRollbackState(CommandRollbackState p_state)
	{
		_board = std::move(p_state.board);
		_units = std::move(p_state.units);
		_rng = p_state.rng;
		_phase = p_state.phase;
		_outcome = p_state.outcome;
		_abortReason = std::move(p_state.abortReason);
		_terminalRecord = std::move(p_state.terminalRecord);
		_activeUnit = p_state.activeUnit;
		_turn = p_state.turn;
		_nextTurn = p_state.nextTurn;
		_resolvedNonEndCommands = p_state.resolvedNonEndCommands;
		_elapsed = p_state.elapsed;
		_playerConfirmed = p_state.playerConfirmed;
		_enemyConfirmed = p_state.enemyConfirmed;
		_shieldAllocator = p_state.shieldAllocator;
		_objectAllocator = p_state.objectAllocator;
		_statusAllocator = p_state.statusAllocator;
		_objects = std::move(p_state.objects);
		_capturedStatusDurations = std::move(p_state.capturedStatusDurations);
		_capturedShieldDurations = std::move(p_state.capturedShieldDurations);
		_capturedObjectDurations = std::move(p_state.capturedObjectDurations);
		_nextAction = p_state.nextAction;
		_nextBatch = p_state.nextBatch;
		_nextEvent = p_state.nextEvent;
		_ordinaryAllocationExhausted = p_state.ordinaryAllocationExhausted;
	}

	bool BattleContext::hasOrdinaryCapacity(std::size_t p_eventCount, bool p_needsAction) const noexcept
	{
		if (_ordinaryAllocationExhausted)
		{
			return false;
		}
		// Guard the conversion/subtraction below as well as the sequence itself.  A malformed
		// resolver must never turn a huge staged vector into an unsigned underflow that appears
		// to have capacity for an ordinary batch.
		if (p_eventCount > static_cast<std::size_t>(CounterMax - ReservedEvents))
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

		std::vector<StagedEvent> staged;
		if (_activeUnit.has_value() && _turn.has_value())
		{
			const BattleUnitId active = *_activeUnit;
			const BattleUnit &activeBattleUnit = unit(active);
			ActivationEnded activationEnded;
			activationEnded.unit = active;
			activationEnded.turn = *_turn;
			activationEnded.finalCell = _board.occupancy().cellOf(active);
			activationEnded.finalActionPoints = activeBattleUnit.actionPoints();
			activationEnded.finalMovementPoints = activeBattleUnit.movementPoints();
			activationEnded.reason = ActivationEndReason::TechnicalAbort;
			staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{.sourceUnit = active}, activationEnded});
			unitMutable(active).setTurnBarFill(BattleTime{});
		}
		_activeUnit = std::nullopt;
		_turn = std::nullopt;
		_resolvedNonEndCommands = 0;
		setTerminal(BattleOutcome::Aborted, p_reason);
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

	void BattleContext::debugSetNextShieldIdForExhaustionTest(std::uint32_t p_next) noexcept
	{
		_shieldAllocator.debugSetNextForExhaustionTest(p_next);
	}
}
