#include "battle/scheduler/stamina_scheduler.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_event.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		[[nodiscard]] bool ready(const BattleUnit &p_unit) noexcept
		{
			return p_unit.isActiveCombatant() && !p_unit.isStunned() &&
				   p_unit.turnBarFill() >= p_unit.effectiveAttributes().stamina;
		}

		[[nodiscard]] std::optional<BattleUnitId> selectReady(const BattleContext &p_context)
		{
			std::optional<BattleUnitId> selected;
			for (const BattleUnitId id : p_context.allUnitsInOrder())
			{
				const BattleUnit &unit = p_context.unit(id);
				if (!ready(unit))
				{
					continue;
				}
				if (!selected.has_value())
				{
					selected = id;
					continue;
				}
				const BattleUnit &current = p_context.unit(*selected);
				if (std::tuple{unit.side() == BattleSide::Player ? 0 : 1, unit.rosterOrder(), id.value()} <
					std::tuple{current.side() == BattleSide::Player ? 0 : 1, current.rosterOrder(), selected->value()})
				{
					selected = id;
				}
			}
			return selected;
		}

		[[nodiscard]] std::optional<BattleTime> timeToReady(const BattleContext &p_context)
		{
			std::optional<BattleTime> result;
			for (const BattleUnitId id : p_context.allUnitsInOrder())
			{
				const BattleUnit &unit = p_context.unit(id);
				if (!unit.isActiveCombatant() || unit.isStunned() || ready(unit))
				{
					continue;
				}
				const BattleTime remaining = unit.effectiveAttributes().stamina - unit.turnBarFill();
				if (!result.has_value() || remaining < *result)
				{
					result = remaining;
				}
			}
			return result;
		}

		[[nodiscard]] std::optional<int> closestDistance(const BattleContext &p_context, BattleUnitId p_source, bool p_allies)
		{
			const std::optional<BoardCell> sourceCell = p_context.board().occupancy().cellOf(p_source);
			if (!sourceCell.has_value())
			{
				return std::nullopt;
			}
			const BattleSide side = p_context.unit(p_source).side();
			std::optional<int> closest;
			for (const BattleUnitId id : p_context.allUnitsInOrder())
			{
				if (id == p_source || (p_context.unit(id).side() == side) != p_allies || !p_context.unit(id).isActiveCombatant())
				{
					continue;
				}
				const std::optional<BoardCell> cell = p_context.board().occupancy().cellOf(id);
				if (!cell.has_value())
				{
					continue;
				}
				const int distance = std::abs(sourceCell->x - cell->x) + std::abs(sourceCell->z - cell->z);
				if (!closest.has_value() || distance < *closest)
				{
					closest = distance;
				}
			}
			return closest;
		}

		void appendStart(BattleContext &p_context, BattleUnitId p_id, std::vector<StagedEvent> &p_events)
		{
			BattleUnit &unit = p_context.unitMutable(p_id);
			const auto resource = [&](BattleResource p_resource, int p_before, int p_after, ResourceChangeReason p_reason) {
				if (p_before == p_after)
				{
					return;
				}
				ResourceChanged changed;
				changed.unit = p_id;
				changed.resource = p_resource;
				changed.requestedDelta = p_after - p_before;
				changed.appliedDelta = changed.requestedDelta;
				changed.before = p_before;
				changed.after = p_after;
				changed.reason = p_reason;
				BattleEventOrigin origin;
				origin.sourceUnit = p_id;
				p_events.push_back({BattleEventCategory::Gameplay, std::move(origin), changed});
			};

			const int oldAp = unit.actionPoints();
			const int maxAp = unit.maxActionPoints();
			unit.setActionPoints(maxAp);
			resource(BattleResource::ActionPoints, oldAp, maxAp, ResourceChangeReason::ActivationRefill);
			const int oldMp = unit.movementPoints();
			const int maxMp = unit.maxMovementPoints();
			unit.setMovementPoints(maxMp);
			resource(BattleResource::MovementPoints, oldMp, maxMp, ResourceChangeReason::ActivationRefill);

			const NextActivationPenalty penalty = unit.nextActivationPenalty();
			const int finalAp = std::max(0, maxAp - penalty.actionPoints);
			unit.setActionPoints(finalAp);
			resource(BattleResource::ActionPoints, maxAp, finalAp, ResourceChangeReason::NextActivationPenaltyConsumption);
			const int finalMp = std::max(0, maxMp - penalty.movementPoints);
			unit.setMovementPoints(finalMp);
			resource(BattleResource::MovementPoints, maxMp, finalMp, ResourceChangeReason::NextActivationPenaltyConsumption);
			unit.clearNextActivationPenalty();

			ActivationStarted started;
			started.unit = p_id;
			started.turn = *p_context.turn();
			started.actionPoints = finalAp;
			started.movementPoints = finalMp;
			started.closestAllyDistance = closestDistance(p_context, p_id, true);
			started.closestEnemyDistance = closestDistance(p_context, p_id, false);
			p_events.push_back({BattleEventCategory::Lifecycle, BattleEventOrigin{.sourceUnit = p_id}, started});
		}
	}

	SchedulerAdvanceResult StaminaScheduler::advanceUntilActivation(BattleContext &p_context)
	{
		SchedulerAdvanceResult result;
		while (true)
		{
			if (const std::optional<BattleUnitId> selected = selectReady(p_context); selected.has_value())
			{
				if (!p_context.canAllocateTurn() || !p_context.hasOrdinaryCapacity(5, false))
				{
					const CommittedBattleBatch aborted = p_context.commitAbortBatch(BattleAbortReason::NumericInvariant, p_context.snapshot());
					result.stop = SchedulerStop::Aborted;
					result.committedBatches.push_back(aborted.id);
					return result;
				}
				const BattleSnapshot before = p_context.snapshot();
				p_context.setPhase(BattlePhase::Activation);
				p_context.setActiveUnit(*selected);
				p_context.setTurn(p_context.allocateTurn());
				p_context.setResolvedNonEndCommands(0);
				std::vector<StagedEvent> events;
				appendStart(p_context, *selected, events);
				const CommittedBattleBatch batch = p_context.commitOrdinaryBatch(BattleBatchKind::Timeline, before, events);
				result.stop = SchedulerStop::ActivationReady;
				result.activeUnit = selected;
				result.committedBatches.push_back(batch.id);
				return result;
			}

			std::optional<BattleTime> delta = timeToReady(p_context);
			if (const std::optional<BattleTime> timeline = p_context.timeUntilNextTimelineBoundary(); timeline.has_value() &&
																									  (!delta.has_value() || *timeline < *delta))
			{
				delta = timeline;
			}
			if (!delta.has_value() || delta->isZero())
			{
				const BattleAbortReason reason = delta.has_value()
													 ? BattleAbortReason::TimelineBoundaryMadeNoProgress
													 : BattleAbortReason::SchedulerNoFutureProgress;
				const CommittedBattleBatch aborted = p_context.commitAbortBatch(reason, p_context.snapshot());
				result.stop = SchedulerStop::Aborted;
				result.committedBatches.push_back(aborted.id);
				return result;
			}
			if (!p_context.hasOrdinaryCapacity(1, false))
			{
				const CommittedBattleBatch aborted = p_context.commitAbortBatch(BattleAbortReason::InternalInvariantViolation, p_context.snapshot());
				result.stop = SchedulerStop::Aborted;
				result.committedBatches.push_back(aborted.id);
				return result;
			}
			const BattleSnapshot before = p_context.snapshot();
			const BattleTime previous = p_context.elapsed();
			p_context.setElapsed(previous + *delta);
			std::vector<StagedEvent> events;
			p_context.advanceTimeline(*delta, events);
			for (const BattleUnitId id : p_context.allUnitsInOrder())
			{
				BattleUnit &unit = p_context.unitMutable(id);
				if (unit.isActiveCombatant() && !unit.isStunned())
				{
					unit.setTurnBarFill(std::min(unit.effectiveAttributes().stamina, unit.turnBarFill() + *delta));
				}
			}
			BattleTimeAdvanced advanced{previous, *delta, p_context.elapsed()};
			events.insert(events.begin(), StagedEvent{BattleEventCategory::Lifecycle, {}, advanced});
			const CommittedBattleBatch batch = p_context.commitOrdinaryBatch(BattleBatchKind::Timeline, before, events);
			result.committedBatches.push_back(batch.id);
		}
	}
}
