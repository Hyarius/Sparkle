#include "battle/effects/effect_resolver.hpp"

#include "battle/status/status_hook_service.hpp"

#include "core/registries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace pg
{
	namespace
	{
		[[nodiscard]] bool isActive(const BattleContext &p_context, BattleUnitId p_id)
		{
			const BattleUnit *unit = p_context.tryUnit(p_id);
			return unit != nullptr && unit->isActiveCombatant();
		}

		[[nodiscard]] std::int64_t checkedAdd(std::int64_t p_left, std::int64_t p_right, const char *p_what)
		{
			if ((p_right > 0 && p_left > std::numeric_limits<std::int64_t>::max() - p_right) ||
				(p_right < 0 && p_left < std::numeric_limits<std::int64_t>::min() - p_right))
			{
				throw std::overflow_error(p_what);
			}
			return p_left + p_right;
		}

		[[nodiscard]] std::int64_t checkedMultiply(std::int64_t p_left, std::int64_t p_right, const char *p_what)
		{
			// The resolver's authored/runtime inputs are non-negative.  Keep the full signed check
			// nevertheless: it makes a corrupted runtime stat fail as a numeric invariant too.
			if (p_left == 0 || p_right == 0)
			{
				return 0;
			}
			if (p_left == -1 && p_right == std::numeric_limits<std::int64_t>::min())
			{
				throw std::overflow_error(p_what);
			}
			if (p_right == -1 && p_left == std::numeric_limits<std::int64_t>::min())
			{
				throw std::overflow_error(p_what);
			}
			const bool positive = (p_left < 0) == (p_right < 0);
			if ((positive && ((p_left > 0 && p_right > std::numeric_limits<std::int64_t>::max() / p_left) ||
							  (p_left < 0 && p_right < std::numeric_limits<std::int64_t>::max() / p_left))) ||
				(!positive && ((p_left > 0 && p_right < std::numeric_limits<std::int64_t>::min() / p_left) ||
							   (p_left < 0 && p_right > std::numeric_limits<std::int64_t>::min() / p_left))))
			{
				throw std::overflow_error(p_what);
			}
			return p_left * p_right;
		}

		[[nodiscard]] int narrowInt(std::int64_t p_value, const char *p_what)
		{
			if (p_value < std::numeric_limits<int>::min() || p_value > std::numeric_limits<int>::max())
			{
				throw std::overflow_error(p_what);
			}
			return static_cast<int>(p_value);
		}

		[[nodiscard]] std::int64_t absoluteDifference(int p_left, int p_right) noexcept
		{
			const std::int64_t difference = static_cast<std::int64_t>(p_left) - static_cast<std::int64_t>(p_right);
			return difference < 0 ? -difference : difference;
		}

		[[nodiscard]] int manhattanDistance(const BoardCell &p_left, const BoardCell &p_right)
		{
			return narrowInt(
				checkedAdd(absoluteDifference(p_left.x, p_right.x), absoluteDifference(p_left.z, p_right.z), "board Manhattan distance overflow"),
				"board Manhattan distance does not fit an event");
		}

		[[nodiscard]] BattleEventOrigin makeOrigin(
			const CastPlan &p_plan,
			const EffectApplication &p_effect,
			std::optional<BattleUnitId> p_target = std::nullopt)
		{
			BattleEventOrigin result;
			result.sourceUnit = p_plan.source;
			result.targetUnit = p_target;
			result.abilityId = p_plan.abilityId;
			result.effectId = p_effect.id;
			return result;
		}

		[[nodiscard]] std::int64_t offenseAmount(
			const BattleUnit *p_source,
			std::int64_t p_base,
			std::int32_t p_strengthRatio,
			std::int32_t p_magicRatio)
		{
			const std::int64_t strength = p_source != nullptr ? p_source->effectiveAttributes().strength : 0;
			const std::int64_t magic = p_source != nullptr ? p_source->effectiveAttributes().magicPower : 0;
			if (p_base < 0 || strength < 0 || magic < 0 || p_strengthRatio < 0 || p_magicRatio < 0)
			{
				throw std::overflow_error("negative offense input");
			}
			return checkedAdd(
					   checkedAdd(
						   checkedMultiply(p_base, 1000, "effect offense overflow"),
						   checkedMultiply(strength, p_strengthRatio, "effect offense overflow"),
						   "effect offense overflow"),
					   checkedMultiply(magic, p_magicRatio, "effect offense overflow"),
					   "effect offense overflow") /
				   1000;
		}

		[[nodiscard]] std::vector<BattleUnitId> unitScope(const CastPlan &p_plan, EffectApplyTo p_applyTo)
		{
			switch (p_applyTo)
			{
			case EffectApplyTo::SourceUnit:
				return {p_plan.source};
			case EffectApplyTo::PrimaryUnit:
				return p_plan.primaryUnit ? std::vector<BattleUnitId>{*p_plan.primaryUnit} : std::vector<BattleUnitId>{};
			case EffectApplyTo::AffectedUnits:
				return p_plan.affectedUnits;
			case EffectApplyTo::AnchorCell:
			case EffectApplyTo::AffectedCells:
				throw std::logic_error("unit payload has a cell execution scope");
			}
			throw std::logic_error("unknown effect scope");
		}

		[[nodiscard]] std::vector<BoardCell> cellScope(const CastPlan &p_plan, EffectApplyTo p_applyTo)
		{
			switch (p_applyTo)
			{
			case EffectApplyTo::AnchorCell:
				return {p_plan.anchor};
			case EffectApplyTo::AffectedCells:
				return p_plan.affectedCells;
			case EffectApplyTo::SourceUnit:
			case EffectApplyTo::PrimaryUnit:
			case EffectApplyTo::AffectedUnits:
				throw std::logic_error("cell payload has a unit execution scope");
			}
			throw std::logic_error("unknown effect scope");
		}

		void skipped(
			std::vector<StagedEvent> &p_events,
			const CastPlan &p_plan,
			const EffectApplication &p_effect,
			std::optional<BattleUnitId> p_target,
			std::optional<BoardCell> p_cell,
			EffectSkipReason p_reason)
		{
			p_events.push_back({BattleEventCategory::Diagnostic, makeOrigin(p_plan, p_effect, p_target), EffectApplicationSkipped{p_effect.id, p_target, p_cell, p_reason}});
		}

		[[nodiscard]] DurationState materializeDuration(const DurationSpec &p_spec, BattleTime p_now)
		{
			return std::visit([p_now](const auto &value) -> DurationState {
				using T = std::decay_t<decltype(value)>;
				if constexpr (std::is_same_v<T, TimelineDurationSpec>)
				{
					return TimelineDurationState{p_now + value.duration};
				}
				else if constexpr (std::is_same_v<T, OwnerActivationsDurationSpec>)
				{
					return OwnerActivationDurationState{value.count};
				}
				else
				{
					return InfiniteDurationState{};
				}
			},
							  p_spec);
		}

		struct LockedDisplacementDirection
		{
			std::size_t neighborIndex = 0;
			int x = 0;
			int z = 0;
		};

		[[nodiscard]] LockedDisplacementDirection displacementDirection(
			const BoardCell &p_source,
			const BoardCell &p_target,
			DisplaceDirection p_direction)
		{
			const std::int64_t dx = static_cast<std::int64_t>(p_target.x) - static_cast<std::int64_t>(p_source.x);
			const std::int64_t dz = static_cast<std::int64_t>(p_target.z) - static_cast<std::int64_t>(p_source.z);
			if (dx == 0 && dz == 0)
			{
				throw std::invalid_argument("no displacement axis");
			}
			std::size_t result = 0;
			if (absoluteDifference(p_target.x, p_source.x) >= absoluteDifference(p_target.z, p_source.z))
			{
				result = dx > 0 ? 0U : 1U; // traversal graph: +X, -X, +Z, -Z
			}
			else
			{
				result = dz > 0 ? 2U : 3U;
			}
			if (p_direction == DisplaceDirection::TowardSource)
			{
				result = result == 0U ? 1U : result == 1U ? 0U
										 : result == 2U	  ? 3U
														  : 2U;
			}
			switch (result)
			{
			case 0U:
				return {.neighborIndex = result, .x = 1, .z = 0};
			case 1U:
				return {.neighborIndex = result, .x = -1, .z = 0};
			case 2U:
				return {.neighborIndex = result, .x = 0, .z = 1};
			case 3U:
				return {.neighborIndex = result, .x = 0, .z = -1};
			default:
				throw std::logic_error("invalid traversal direction index");
			}
		}
	}

	bool EffectResolver::supports(const EffectPayload &p_payload) noexcept
	{
		return std::holds_alternative<DamageEffectSpec>(p_payload) || std::holds_alternative<HealEffectSpec>(p_payload) ||
			   std::holds_alternative<ApplyShieldEffectSpec>(p_payload) || std::holds_alternative<ApplyStatusEffectSpec>(p_payload) ||
			   std::holds_alternative<RemoveStatusEffectSpec>(p_payload) || std::holds_alternative<CleanseEffectSpec>(p_payload) ||
			   std::holds_alternative<PlaceObjectEffectSpec>(p_payload) || std::holds_alternative<RemoveObjectsEffectSpec>(p_payload) ||
			   std::holds_alternative<ChangeResourceEffectSpec>(p_payload) ||
			   std::holds_alternative<ApplyNextActivationPenaltyEffectSpec>(p_payload) || std::holds_alternative<AdjustTurnBarEffectSpec>(p_payload) ||
			   std::holds_alternative<DisplaceEffectSpec>(p_payload) || std::holds_alternative<SwapWithSourceEffectSpec>(p_payload) ||
			   std::holds_alternative<TeleportSourceEffectSpec>(p_payload);
	}

	bool EffectResolver::supportsAll(const AbilityDefinition &p_ability) noexcept
	{
		return std::ranges::all_of(p_ability.effects, [](const EffectApplication &p_effect) {
			return supports(p_effect.payload);
		});
	}

	bool EffectResolver::applySpatialStep(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		BattleUnitId p_unit,
		BoardCell p_from,
		BoardCell p_to,
		int p_enteredCellCost,
		SpatialMoveCause p_cause,
		const BattleEventOrigin &p_origin)
	{
		if (p_context.blocksMovement(p_to))
		{
			return false;
		}
		if (!p_context.moveUnitOccupancy(p_unit, p_to))
		{
			return false;
		}
		p_events.push_back({BattleEventCategory::Gameplay, p_origin, UnitMovementStep{p_unit, p_from, p_to, p_enteredCellCost, p_cause}});
		_unitLeftCellSeam(p_context, p_events, p_unit, p_from);
		if (isActive(p_context, p_unit) && p_context.board().occupancy().cellOf(p_unit) == p_to)
		{
			_unitEnteredCellSeam(p_context, p_events, p_unit, p_to);
		}
		return true;
	}

	void EffectResolver::finishSpatialMove(BattleContext &p_context, std::vector<StagedEvent> &p_events, BattleUnitId p_unit, BoardCell p_finalCell)
	{
		if (isActive(p_context, p_unit) && p_context.board().occupancy().cellOf(p_unit) == p_finalCell)
		{
			_unitEndedMoveOnCellSeam(p_context, p_events, p_unit, p_finalCell);
		}
	}

	void EffectResolver::_applyDamage(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		std::vector<BattleUnitId> &p_pendingDefeats,
		const CastPlan &p_plan,
		const EffectApplication &p_effect,
		BattleUnitId p_targetId,
		const DamageEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const std::int64_t raw = offenseAmount(p_context.tryUnit(p_plan.source), p_spec.base, p_spec.strengthRatioPermille, p_spec.magicPowerRatioPermille);
		const std::int64_t defense = p_spec.kind == DamageKind::Physical ? target.effectiveAttributes().armor : target.effectiveAttributes().resistance;
		const std::int64_t scale = p_context.registries().gameRules().battle.mitigationScale;
		if (defense < 0 || scale <= 0)
		{
			throw std::overflow_error("invalid mitigation inputs");
		}
		const std::int64_t mitigated = checkedMultiply(raw, scale, "damage mitigation overflow") /
									   checkedAdd(defense, scale, "damage mitigation denominator overflow");
		const int computed = narrowInt(mitigated, "computed damage does not fit an event");
		const int before = target.health();
		if (before < 0)
		{
			throw std::overflow_error("negative battle health");
		}
		int remaining = computed;
		int absorbed = 0;
		bool anyShield = false;
		bool matchingShield = false;
		for (const BattleShield &shield : target.shields())
		{
			if (shield.remainingAmount > 0)
			{
				anyShield = true;
				matchingShield = matchingShield || shield.kind == p_spec.kind;
			}
		}
		for (auto it = target.shieldsMutable().begin(); it != target.shieldsMutable().end() && remaining > 0;)
		{
			if (it->kind != p_spec.kind || it->remainingAmount <= 0)
			{
				++it;
				continue;
			}
			const int requested = remaining;
			const int appliedShield = narrowInt(std::min<std::int64_t>(remaining, it->remainingAmount), "shield absorption does not fit an event");
			it->remainingAmount -= appliedShield;
			remaining -= appliedShield;
			absorbed = narrowInt(checkedAdd(absorbed, appliedShield, "shield absorption total overflow"), "shield absorption total does not fit an event");
			p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), ShieldAbsorbed{p_plan.source, p_targetId, it->id, it->kind, requested, appliedShield, it->source, it->sourceAbilityId, it->sourceEffectId}});
			if (it->remainingAmount == 0)
			{
				p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), ShieldBroken{p_plan.source, p_targetId, it->id, it->kind, it->source, it->sourceAbilityId, it->sourceEffectId}});
				it = target.shieldsMutable().erase(it);
			}
			else
			{
				++it;
			}
		}
		const int applied = std::min(before, remaining);
		target.setHealth(before - applied);

		Damage event;
		event.source = p_plan.source;
		event.target = p_targetId;
		event.abilityId = p_plan.abilityId;
		event.effectId = p_effect.id;
		event.kind = p_spec.kind;
		event.computedDamage = computed;
		event.appliedToShield = absorbed;
		event.targetHadAnyShield = anyShield;
		event.targetHadMatchingShield = matchingShield;
		event.appliedToHealth = applied;
		event.healthBefore = before;
		event.healthAfter = target.health();
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), event});
		if (before > 0 && target.health() == 0)
		{
			p_pendingDefeats.push_back(p_targetId);
		}
		// The direct reaction order is explicit even while every seam is a no-op: dealt first,
		// then taken.  Step 10 fills these without returning through public event publication.
		_afterDamageDealtSeam(p_context, p_events, p_plan, p_effect, p_plan.source, p_targetId);
		_afterDamageTakenSeam(p_context, p_events, p_plan, p_effect, p_plan.source, p_targetId);
	}

	void EffectResolver::_applyShield(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const ApplyShieldEffectSpec &p_spec)
	{
		const int amount = narrowInt(p_spec.amount, "shield amount does not fit an event");
		if (amount <= 0)
		{
			throw std::overflow_error("non-positive shield amount");
		}
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const BattleShieldId id = p_context.allocateShieldId();
		target.shieldsMutable().push_back(BattleShield{id, p_spec.kind, amount, materializeDuration(p_spec.duration, p_context.elapsed()), p_plan.source, p_plan.abilityId, p_effect.id});
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), ShieldApplied{p_plan.source, p_targetId, id, p_spec.kind, amount, amount, snapshotDuration(target.shields().back().duration), p_plan.abilityId, p_effect.id}});
	}

	void EffectResolver::_applyStatus(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan,
		const EffectApplication &p_effect,
		BattleUnitId p_targetId,
		const ApplyStatusEffectSpec &p_spec)
	{
		const StatusDefinition *definition = p_context.registries().statuses().tryGet(p_spec.status);
		if (definition == nullptr)
		{
			throw std::logic_error("validated applyStatus references an unavailable status");
		}
		const std::uint32_t requested = p_spec.stacks;
		const std::uint32_t incomingStacks = std::min(requested, definition->maxStacks);
		const DurationState incomingDuration = materializeDuration(p_spec.duration, p_context.elapsed());
		BattleUnit &target = p_context.unitMutable(p_targetId);
		auto &transients = target.transientStatusesMutable();
		auto current = std::ranges::find_if(transients, [&p_spec](const BattleStatusInstance &status) {
			return status.definitionId == p_spec.status;
		});
		const auto sameDuration = [](const DurationState &p_a, const DurationState &p_b) {
			if (p_a.index() != p_b.index())
			{
				return false;
			}
			return std::visit([](const auto &a, const auto &b) -> bool {
				using A = std::decay_t<decltype(a)>;
				using B = std::decay_t<decltype(b)>;
				if constexpr (std::is_same_v<A, B>)
				{
					if constexpr (std::is_same_v<A, TimelineDurationState>)
					{
						return a.expiresAt == b.expiresAt;
					}
					if constexpr (std::is_same_v<A, OwnerActivationDurationState>)
					{
						return a.remainingActivations == b.remainingActivations;
					}
					return true;
				}
				return false;
			},
							  p_a,
							  p_b);
		};
		const auto refreshed = [&definition, &incomingDuration, &p_context](const DurationState &p_current) -> DurationState {
			if (definition->durationRefreshPolicy == DurationRefreshPolicy::Replace)
			{
				return incomingDuration;
			}
			if (std::holds_alternative<InfiniteDurationState>(p_current) || std::holds_alternative<InfiniteDurationState>(incomingDuration))
			{
				return InfiniteDurationState{};
			}
			if (const auto *oldTimeline = std::get_if<TimelineDurationState>(&p_current))
			{
				const auto *newTimeline = std::get_if<TimelineDurationState>(&incomingDuration);
				if (newTimeline == nullptr)
				{
					throw std::logic_error("mixed finite duration kinds escaped validation");
				}
				return definition->durationRefreshPolicy == DurationRefreshPolicy::KeepLonger
						   ? DurationState{TimelineDurationState{std::max(oldTimeline->expiresAt, newTimeline->expiresAt)}}
						   : DurationState{TimelineDurationState{oldTimeline->expiresAt + (newTimeline->expiresAt - p_context.elapsed())}};
			}
			const auto *oldActivations = std::get_if<OwnerActivationDurationState>(&p_current);
			const auto *newActivations = std::get_if<OwnerActivationDurationState>(&incomingDuration);
			if (oldActivations == nullptr || newActivations == nullptr)
			{
				throw std::logic_error("mixed finite duration kinds escaped validation");
			}
			if (definition->durationRefreshPolicy == DurationRefreshPolicy::KeepLonger)
			{
				return OwnerActivationDurationState{std::max(oldActivations->remainingActivations, newActivations->remainingActivations)};
			}
			if (oldActivations->remainingActivations > std::numeric_limits<std::uint32_t>::max() - newActivations->remainingActivations)
			{
				throw std::overflow_error("owner activation duration extension overflowed");
			}
			return OwnerActivationDurationState{oldActivations->remainingActivations + newActivations->remainingActivations};
		};

		int previous = 0;
		int applied = 0;
		BattleStatusInstanceId instanceId;
		DurationState resultingDuration = incomingDuration;
		if (current == transients.end())
		{
			BattleStatusInstance instance;
			instance.id = p_context.allocateStatusInstanceId();
			instance.definitionId = definition->id;
			instance.origin = BattleStatusOrigin::TransientEffect;
			instance.stacks = incomingStacks;
			instance.duration = incomingDuration;
			instance.appliedBy = p_plan.source;
			instance.sourceAbilityId = p_plan.abilityId;
			instance.sourceEffectId = p_effect.id;
			instanceId = instance.id;
			applied = static_cast<int>(incomingStacks);
			transients.push_back(std::move(instance));
		}
		else
		{
			previous = static_cast<int>(current->stacks);
			const std::uint32_t nextStacks = definition->reapplyPolicy == StatusReapplyPolicy::AddStacks
												 ? std::min(definition->maxStacks, static_cast<std::uint32_t>(checkedAdd(current->stacks, requested, "status stack addition overflowed")))
												 : incomingStacks;
			resultingDuration = refreshed(current->duration);
			applied = static_cast<int>(nextStacks) - previous;
			instanceId = current->id;
			const bool durationChanged = !sameDuration(current->duration, resultingDuration);
			if (applied == 0 && !durationChanged)
			{
				skipped(p_events, p_plan, p_effect, p_targetId, std::nullopt, EffectSkipReason::NoAppliedChange);
				return;
			}
			current->stacks = nextStacks;
			current->duration = resultingDuration;
			current->appliedBy = p_plan.source;
			current->sourceAbilityId = p_plan.abilityId;
			current->sourceEffectId = p_effect.id;
		}
		p_context.reconcileEffectiveStats(p_targetId, &p_events);
		StatusApplied event;
		event.source = p_plan.source;
		event.target = p_targetId;
		event.statusId = definition->id;
		event.requestedStacks = static_cast<int>(requested);
		event.appliedStacks = applied;
		event.resultingStacks = previous + applied;
		event.tags = definition->tags;
		event.instanceId = instanceId;
		event.duration = snapshotDuration(resultingDuration);
		event.origin = BattleStatusOrigin::TransientEffect;
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), std::move(event)});
		StatusHookService::dispatchStatusHook(p_context, p_events, p_targetId, StatusHook::Applied, p_targetId, instanceId);
	}

	void EffectResolver::_removeStatus(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan,
		const EffectApplication &p_effect,
		BattleUnitId p_targetId,
		const RemoveStatusEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		auto &transients = target.transientStatusesMutable();
		const auto found = std::ranges::find_if(transients, [&p_spec](const BattleStatusInstance &status) {
			return status.definitionId == p_spec.status;
		});
		if (found == transients.end())
		{
			skipped(p_events, p_plan, p_effect, p_targetId, std::nullopt, EffectSkipReason::NoAppliedChange);
			return;
		}
		const StatusDefinition *definition = p_context.registries().statuses().tryGet(found->definitionId);
		const std::uint32_t previous = found->stacks;
		const std::uint32_t removed = std::min(previous, p_spec.stacks);
		const std::uint32_t remaining = previous - removed;
		const BattleStatusInstanceId instanceId = found->id;
		const std::optional<BattleUnitId> originalApplier = found->appliedBy;
		std::optional<BattleStatusInstance> erased;
		if (remaining == 0)
		{
			erased = *found;
			transients.erase(found);
		}
		else
		{
			found->stacks = remaining;
		}
		p_context.reconcileEffectiveStats(p_targetId, &p_events);
		StatusRemoved event;
		event.source = p_plan.source;
		event.originalApplier = originalApplier;
		event.target = p_targetId;
		event.statusId = p_spec.status;
		event.requestedStacks = static_cast<int>(p_spec.stacks);
		event.removedStacks = static_cast<int>(removed);
		event.remainingStacks = static_cast<int>(remaining);
		event.reason = StatusRemovalReason::ExplicitEffect;
		event.tags = definition == nullptr ? std::vector<std::string>{} : definition->tags;
		event.instanceId = instanceId;
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), std::move(event)});
		if (erased.has_value())
		{
			StatusHookService::dispatchRemovedHook(p_context, p_events, p_targetId, *erased);
		}
	}

	void EffectResolver::_cleanse(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan,
		const EffectApplication &p_effect,
		BattleUnitId p_targetId,
		const CleanseEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		std::vector<BattleStatusInstanceId> matches;
		for (const BattleStatusInstance &status : target.transientStatuses())
		{
			const StatusDefinition *definition = p_context.registries().statuses().tryGet(status.definitionId);
			if (definition != nullptr && std::ranges::any_of(p_spec.tags, [&definition](const std::string &tag) {
					return std::ranges::find(definition->tags, tag) != definition->tags.end();
				}))
			{
				matches.push_back(status.id);
			}
		}
		for (const BattleStatusInstanceId id : matches)
		{
			auto &transients = target.transientStatusesMutable();
			auto found = std::ranges::find_if(transients, [id](const BattleStatusInstance &status) {
				return status.id == id;
			});
			if (found == transients.end())
			{
				continue;
			}
			const StatusDefinition *definition = p_context.registries().statuses().tryGet(found->definitionId);
			const BattleStatusInstance removed = *found;
			transients.erase(found);
			p_context.reconcileEffectiveStats(p_targetId, &p_events);
			StatusRemoved event;
			event.source = p_plan.source;
			event.originalApplier = removed.appliedBy;
			event.target = p_targetId;
			event.statusId = removed.definitionId;
			event.requestedStacks = static_cast<int>(removed.stacks);
			event.removedStacks = static_cast<int>(removed.stacks);
			event.reason = StatusRemovalReason::Cleanse;
			event.tags = definition == nullptr ? std::vector<std::string>{} : definition->tags;
			event.instanceId = removed.id;
			p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), std::move(event)});
			StatusHookService::dispatchRemovedHook(p_context, p_events, p_targetId, removed);
		}
		if (matches.empty())
		{
			skipped(p_events, p_plan, p_effect, p_targetId, std::nullopt, EffectSkipReason::NoAppliedChange);
		}
	}

	void EffectResolver::_placeObject(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan,
		const EffectApplication &p_effect,
		BoardCell p_cell,
		const PlaceObjectEffectSpec &p_spec)
	{
		const BattleObjectDefinition *definition = p_context.registries().battleObjects().tryGet(p_spec.object);
		if (definition == nullptr || !p_context.board().isStandable(p_cell))
		{
			skipped(p_events, p_plan, p_effect, std::nullopt, p_cell, EffectSkipReason::DestinationInvalid);
			return;
		}
		for (const BattleObjectId id : p_context.board().occupancy().objectsAt(p_cell))
		{
			const BattleObjectInstance *other = p_context.tryObject(id);
			if (other != nullptr && other->definitionId == definition->id && other->creator == std::optional<BattleUnitId>(p_plan.source))
			{
				skipped(p_events, p_plan, p_effect, std::nullopt, p_cell, EffectSkipReason::NoAppliedChange);
				return;
			}
		}
		if (definition->blocksMovement && (p_context.board().occupancy().unitAt(p_cell).has_value() || p_context.blocksMovement(p_cell)))
		{
			skipped(p_events, p_plan, p_effect, std::nullopt, p_cell, EffectSkipReason::DestinationBlocked);
			return;
		}
		BattleObjectInstance object;
		object.id = p_context.allocateObjectId();
		object.definitionId = definition->id;
		object.creator = p_plan.source;
		object.creatorSide = p_context.unit(p_plan.source).side();
		object.cell = p_cell;
		object.duration = materializeDuration(p_spec.duration, p_context.elapsed());
		object.sourceAbilityId = p_plan.abilityId;
		object.sourceEffectId = p_effect.id;
		for (const BattleObjectTriggerSpec &trigger : definition->triggers)
		{
			object.triggerStates.push_back({trigger.id, 0});
		}
		const BattleObjectId id = object.id;
		if (!p_context.placeObject(std::move(object)))
		{
			throw std::logic_error("validated object placement was rejected by occupancy");
		}
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect), BattleObjectPlaced{id, definition->id, p_plan.source, p_cell}});
	}

	void EffectResolver::_removeObjects(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan,
		const EffectApplication &p_effect,
		BoardCell p_cell,
		const RemoveObjectsEffectSpec &p_spec)
	{
		const std::vector<BattleObjectId> ids(p_context.board().occupancy().objectsAt(p_cell).begin(), p_context.board().occupancy().objectsAt(p_cell).end());
		bool any = false;
		for (const BattleObjectId id : ids)
		{
			const BattleObjectInstance *object = p_context.tryObject(id);
			const BattleObjectDefinition *definition = object == nullptr ? nullptr : p_context.registries().battleObjects().tryGet(object->definitionId);
			if (object == nullptr || definition == nullptr || !std::ranges::any_of(p_spec.tags, [&definition](const std::string &tag) {
					return std::ranges::find(definition->tags, tag) != definition->tags.end();
				}))
			{
				continue;
			}
			const BattleObjectInstance copy = *object;
			(void)p_context.removeObject(id);
			p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect), BattleObjectRemoved{id, copy.definitionId, copy.cell, BattleObjectRemovalReason::ExplicitEffect}});
			any = true;
		}
		if (!any)
		{
			skipped(p_events, p_plan, p_effect, std::nullopt, p_cell, EffectSkipReason::NoAppliedChange);
		}
	}

	void EffectResolver::_applyHealing(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const HealEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const int requested = narrowInt(offenseAmount(p_context.tryUnit(p_plan.source), p_spec.base, p_spec.strengthRatioPermille, p_spec.magicPowerRatioPermille), "requested healing does not fit an event");
		const int before = target.health();
		const int missing = narrowInt(std::max<std::int64_t>(0, checkedAdd(target.maxHealth(), -static_cast<std::int64_t>(before), "healing maximum overflow")), "healing maximum does not fit an event");
		const int applied = std::min(requested, missing);
		target.setHealth(narrowInt(checkedAdd(before, applied, "healing health overflow"), "healing health does not fit an event"));
		Healing event;
		event.source = p_plan.source;
		event.target = p_targetId;
		event.abilityId = p_plan.abilityId;
		event.effectId = p_effect.id;
		event.requestedHealing = requested;
		event.appliedHealing = applied;
		event.healthBefore = before;
		event.healthAfter = target.health();
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), event});
		_afterHealingDoneSeam(p_context, p_events, p_plan, p_effect, p_plan.source, p_targetId);
		_afterHealingReceivedSeam(p_context, p_events, p_plan, p_effect, p_plan.source, p_targetId);
	}

	void EffectResolver::_applyResourceChange(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const ChangeResourceEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const int requested = narrowInt(p_spec.delta, "resource delta does not fit an event");
		const int before = p_spec.resource == BattleResource::ActionPoints ? target.actionPoints() : target.movementPoints();
		const int maximum = p_spec.resource == BattleResource::ActionPoints ? target.maxActionPoints() : target.maxMovementPoints();
		const std::int64_t unclamped = checkedAdd(before, p_spec.delta, "resource change overflow");
		const int after = narrowInt(std::clamp(unclamped, std::int64_t{0}, static_cast<std::int64_t>(maximum)), "resource result does not fit an event");
		if (p_spec.resource == BattleResource::ActionPoints)
		{
			target.setActionPoints(after);
		}
		else
		{
			target.setMovementPoints(after);
		}
		const int applied = narrowInt(checkedAdd(after, -static_cast<std::int64_t>(before), "resource applied delta overflow"), "resource applied delta does not fit an event");
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), ResourceChanged{p_targetId, p_spec.resource, requested, applied, before, after, ResourceChangeReason::Effect}});
	}

	void EffectResolver::_applyNextActivationPenalty(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const ApplyNextActivationPenaltyEffectSpec &p_spec)
	{
		const int requested = narrowInt(p_spec.amount, "next-activation penalty does not fit an event");
		if (requested < 0)
		{
			throw std::overflow_error("negative next-activation penalty");
		}
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const int before = p_spec.resource == BattleResource::ActionPoints ? target.nextActivationPenalty().actionPoints : target.nextActivationPenalty().movementPoints;
		const int after = narrowInt(checkedAdd(before, requested, "next-activation penalty overflow"), "next-activation penalty does not fit an event");
		target.addNextActivationPenalty(p_spec.resource, requested);
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), NextActivationPenaltyApplied{p_targetId, p_spec.resource, requested, requested, before, after}});
	}

	void EffectResolver::_applyTurnBarAdjustment(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const AdjustTurnBarEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const BattleTime before = target.turnBarFill();
		const BattleTime after = clamp(before + p_spec.delta, BattleTime{}, target.effectiveAttributes().stamina);
		target.setTurnBarFill(after);
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), TurnBarAdjusted{p_targetId, p_spec.delta, after - before, before, after}});
	}

	void EffectResolver::_applyDisplace(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_target, const DisplaceEffectSpec &p_spec)
	{
		const std::optional<BoardCell> source = p_context.board().occupancy().cellOf(p_plan.source);
		const std::optional<BoardCell> initial = p_context.board().occupancy().cellOf(p_target);
		if (!initial)
		{
			skipped(p_events, p_plan, p_effect, p_target, std::nullopt, EffectSkipReason::TargetNoLongerPlaced);
			return;
		}
		if (!source)
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, *initial, EffectSkipReason::SourceNotLiving);
			return;
		}

		LockedDisplacementDirection direction;
		try
		{
			direction = displacementDirection(*source, *initial, p_spec.direction);
		} catch (const std::invalid_argument &)
		{
			p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_target), UnitDisplaced{p_plan.source, p_target, p_spec.direction == DisplaceDirection::AwayFromSource ? DisplacementDirection::Away : DisplacementDirection::Toward, *initial, *initial, 0, 0, p_spec.distance, 0}});
			skipped(p_events, p_plan, p_effect, p_target, *initial, EffectSkipReason::NoDirectionalAxis);
			return;
		}

		BoardCell current = *initial;
		int moved = 0;
		for (; moved < p_spec.distance; ++moved)
		{
			const TraversalGraph::Node *node = p_context.board().navigation().tryGetNode(current);
			if (node == nullptr || !node->neighbors[direction.neighborIndex].has_value())
			{
				break;
			}
			const BoardCell next = p_context.board().navigation().node(*node->neighbors[direction.neighborIndex]).position;
			if (p_context.board().occupancy().unitAt(next).has_value() || p_context.blocksMovement(next))
			{
				break;
			}
			if (!applySpatialStep(
					p_context,
					p_events,
					p_target,
					current,
					next,
					0,
					SpatialMoveCause::Displacement,
					makeOrigin(p_plan, p_effect, p_target)))
			{
				throw std::logic_error("validated displacement transition was rejected by occupancy");
			}
			current = next;
		}
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_target), UnitDisplaced{p_plan.source, p_target, p_spec.direction == DisplaceDirection::AwayFromSource ? DisplacementDirection::Away : DisplacementDirection::Toward, *initial, current, direction.x, direction.z, p_spec.distance, moved}});
		if (moved == 0)
		{
			skipped(p_events, p_plan, p_effect, p_target, current, EffectSkipReason::DestinationBlocked);
		}
		else
		{
			finishSpatialMove(p_context, p_events, p_target, current);
		}
	}

	void EffectResolver::_applySwap(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect)
	{
		if (p_effect.applyTo != EffectApplyTo::PrimaryUnit)
		{
			throw std::logic_error("swap payload was not validated with a primary-unit scope");
		}
		if (!p_plan.primaryUnit)
		{
			skipped(p_events, p_plan, p_effect, std::nullopt, std::nullopt, EffectSkipReason::MissingScopeValue);
			return;
		}
		const BattleUnitId target = *p_plan.primaryUnit;
		if (target == p_plan.source)
		{
			skipped(p_events, p_plan, p_effect, target, std::nullopt, EffectSkipReason::NoAppliedChange);
			return;
		}
		if (!isActive(p_context, p_plan.source))
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, std::nullopt, EffectSkipReason::SourceNotLiving);
			return;
		}
		if (!isActive(p_context, target))
		{
			skipped(p_events, p_plan, p_effect, target, std::nullopt, EffectSkipReason::TargetNotLiving);
			return;
		}
		const std::optional<BoardCell> source = p_context.board().occupancy().cellOf(p_plan.source);
		const std::optional<BoardCell> targetCell = p_context.board().occupancy().cellOf(target);
		if (!source || !targetCell)
		{
			skipped(p_events, p_plan, p_effect, target, targetCell, EffectSkipReason::TargetNoLongerPlaced);
			return;
		}
		if (!p_context.swapUnitsOccupancy(p_plan.source, target))
		{
			throw std::logic_error("validated swap was rejected by occupancy");
		}
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, target), UnitsSwapped{p_plan.source, target, *source, *targetCell, *targetCell, *source}});

		struct Transition
		{
			BattleUnitId unit;
			BoardCell from;
			BoardCell to;
		};
		std::array<Transition, 2> transitions{{{p_plan.source, *source, *targetCell}, {target, *targetCell, *source}}};
		std::ranges::sort(transitions, {}, &Transition::unit);
		for (const Transition &transition : transitions)
		{
			p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, transition.unit), UnitMovementStep{transition.unit, transition.from, transition.to, 0, SpatialMoveCause::Swap}});
		}
		for (const Transition &transition : transitions)
		{
			_unitLeftCellSeam(p_context, p_events, transition.unit, transition.from);
		}
		for (const Transition &transition : transitions)
		{
			if (isActive(p_context, transition.unit) && p_context.board().occupancy().cellOf(transition.unit) == transition.to)
			{
				_unitEnteredCellSeam(p_context, p_events, transition.unit, transition.to);
			}
		}
		for (const Transition &transition : transitions)
		{
			if (isActive(p_context, transition.unit) && p_context.board().occupancy().cellOf(transition.unit) == transition.to)
			{
				_unitEndedMoveOnCellSeam(p_context, p_events, transition.unit, transition.to);
			}
		}
	}

	void EffectResolver::_applyTeleport(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const EffectApplication &p_effect)
	{
		if (p_effect.applyTo != EffectApplyTo::AnchorCell)
		{
			throw std::logic_error("teleport payload was not validated with an anchor-cell scope");
		}
		const BoardCell destination = cellScope(p_plan, p_effect.applyTo).front();
		if (!isActive(p_context, p_plan.source))
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, destination, EffectSkipReason::SourceNotLiving);
			return;
		}
		const std::optional<BoardCell> source = p_context.board().occupancy().cellOf(p_plan.source);
		if (!source)
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, destination, EffectSkipReason::TargetNoLongerPlaced);
			return;
		}
		if (!p_context.board().isStandable(destination))
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, destination, EffectSkipReason::DestinationInvalid);
			return;
		}
		if (*source == destination)
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, destination, EffectSkipReason::NoAppliedChange);
			return;
		}
		if (p_context.board().occupancy().unitAt(destination).has_value() || p_context.blocksMovement(destination))
		{
			skipped(p_events, p_plan, p_effect, p_plan.source, destination, EffectSkipReason::DestinationBlocked);
			return;
		}
		if (!applySpatialStep(
				p_context,
				p_events,
				p_plan.source,
				*source,
				destination,
				0,
				SpatialMoveCause::Teleport,
				makeOrigin(p_plan, p_effect, p_plan.source)))
		{
			throw std::logic_error("validated teleport was rejected by occupancy");
		}
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_plan.source), UnitTeleported{p_plan.source, *source, destination, manhattanDistance(*source, destination)}});
		finishSpatialMove(p_context, p_events, p_plan.source, destination);
	}

	void EffectResolver::resolveAbility(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const AbilityDefinition &p_ability)
	{
		for (const EffectApplication &effect : p_ability.effects)
		{
			if (effect.requiresLivingSource && !isActive(p_context, p_plan.source))
			{
				skipped(p_events, p_plan, effect, p_plan.source, std::nullopt, EffectSkipReason::SourceNotLiving);
				continue;
			}

			std::vector<BattleUnitId> pendingDefeats;
			if (std::holds_alternative<SwapWithSourceEffectSpec>(effect.payload))
			{
				_applySwap(p_context, p_events, p_plan, effect);
			}
			else if (std::holds_alternative<TeleportSourceEffectSpec>(effect.payload))
			{
				_applyTeleport(p_context, p_events, p_plan, effect);
			}
			else if (const auto *placeObject = std::get_if<PlaceObjectEffectSpec>(&effect.payload))
			{
				for (const BoardCell &cell : cellScope(p_plan, effect.applyTo))
				{
					_placeObject(p_context, p_events, p_plan, effect, cell, *placeObject);
				}
			}
			else if (const auto *removeObjects = std::get_if<RemoveObjectsEffectSpec>(&effect.payload))
			{
				for (const BoardCell &cell : cellScope(p_plan, effect.applyTo))
				{
					_removeObjects(p_context, p_events, p_plan, effect, cell, *removeObjects);
				}
			}
			else
			{
				const std::vector<BattleUnitId> targets = unitScope(p_plan, effect.applyTo);
				if (effect.applyTo == EffectApplyTo::PrimaryUnit && targets.empty())
				{
					skipped(p_events, p_plan, effect, std::nullopt, std::nullopt, EffectSkipReason::MissingScopeValue);
				}
				for (const BattleUnitId target : targets)
				{
					if (!isActive(p_context, target))
					{
						skipped(p_events, p_plan, effect, target, std::nullopt, EffectSkipReason::TargetNotLiving);
						continue;
					}
					std::visit([&](const auto &payload) {
						using Payload = std::decay_t<decltype(payload)>;
						if constexpr (std::is_same_v<Payload, DamageEffectSpec>)
						{
							_applyDamage(p_context, p_events, pendingDefeats, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, HealEffectSpec>)
						{
							_applyHealing(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, ApplyShieldEffectSpec>)
						{
							_applyShield(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, ApplyStatusEffectSpec>)
						{
							_applyStatus(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, RemoveStatusEffectSpec>)
						{
							_removeStatus(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, CleanseEffectSpec>)
						{
							_cleanse(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, ChangeResourceEffectSpec>)
						{
							_applyResourceChange(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, ApplyNextActivationPenaltyEffectSpec>)
						{
							_applyNextActivationPenalty(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, AdjustTurnBarEffectSpec>)
						{
							_applyTurnBarAdjustment(p_context, p_events, p_plan, effect, target, payload);
						}
						else if constexpr (std::is_same_v<Payload, DisplaceEffectSpec>)
						{
							_applyDisplace(p_context, p_events, p_plan, effect, target, payload);
						}
						else
						{
							throw std::logic_error("resolver received a payload outside its supported catalog");
						}
					},
							   effect.payload);
				}
			}

			// A complete authored application observes its captured order before any HP-zero unit is
			// removed.  Final removal is canonical by session unit order, while the stored origin stays
			// the effect that made the positive-to-zero transition.
			for (const BattleUnitId id : p_context.allUnitsInOrder())
			{
				if (std::ranges::find(pendingDefeats, id) != pendingDefeats.end())
				{
					p_context.removeUnit(id, RemovalReason::Defeated, makeOrigin(p_plan, effect, id), p_events);
				}
			}
		}
		_afterAbilityCastReaction(p_context, p_events, p_plan, p_ability);
	}

	void EffectResolver::_afterDamageDealtSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &, const EffectApplication &, BattleUnitId p_source, BattleUnitId p_target)
	{
		StatusHookService::dispatchStatusHook(p_context, p_events, p_source, StatusHook::AfterDamageDealt, p_target);
	}

	void EffectResolver::_afterDamageTakenSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &, const EffectApplication &, BattleUnitId p_source, BattleUnitId p_target)
	{
		StatusHookService::dispatchStatusHook(p_context, p_events, p_target, StatusHook::AfterDamageTaken, p_source);
	}

	void EffectResolver::_afterHealingDoneSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &, const EffectApplication &, BattleUnitId p_source, BattleUnitId p_target)
	{
		StatusHookService::dispatchStatusHook(p_context, p_events, p_source, StatusHook::AfterHealingDone, p_target);
	}

	void EffectResolver::_afterHealingReceivedSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &, const EffectApplication &, BattleUnitId p_source, BattleUnitId p_target)
	{
		StatusHookService::dispatchStatusHook(p_context, p_events, p_target, StatusHook::AfterHealingReceived, p_source);
	}

	void EffectResolver::_afterAbilityCastReaction(BattleContext &p_context, std::vector<StagedEvent> &p_events, const CastPlan &p_plan, const AbilityDefinition &)
	{
		StatusHookService::dispatchStatusHook(p_context, p_events, p_plan.source, StatusHook::AfterAbilityCast, p_plan.source);
	}

	void EffectResolver::_unitLeftCellSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, BattleUnitId p_unit, BoardCell p_cell)
	{
		StatusHookService::dispatchObjectTrigger(p_context, p_events, BattleObjectTrigger::UnitLeftCell, p_unit, p_cell);
	}

	void EffectResolver::_unitEnteredCellSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, BattleUnitId p_unit, BoardCell p_cell)
	{
		StatusHookService::dispatchObjectTrigger(p_context, p_events, BattleObjectTrigger::UnitEnteredCell, p_unit, p_cell);
	}

	void EffectResolver::_unitEndedMoveOnCellSeam(BattleContext &p_context, std::vector<StagedEvent> &p_events, BattleUnitId p_unit, BoardCell p_cell)
	{
		StatusHookService::dispatchObjectTrigger(p_context, p_events, BattleObjectTrigger::UnitEndedMoveOnCell, p_unit, p_cell);
	}
}
