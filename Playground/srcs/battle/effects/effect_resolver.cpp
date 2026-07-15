#include "battle/effects/effect_resolver.hpp"

#include "core/registries.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		[[nodiscard]] bool isActive(const BattleContext &p_context, BattleUnitId p_id)
		{
			const BattleUnit *unit = p_context.tryUnit(p_id);
			return unit != nullptr && unit->isActiveCombatant();
		}

		[[nodiscard]] BattleEventOrigin makeOrigin(
			const CastPlan &p_plan,
			const EffectApplication &p_effect,
			BattleUnitId p_target)
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
			const auto add = [](std::int64_t p_left, std::int64_t p_right) {
				if (p_right > std::numeric_limits<std::int64_t>::max() - p_left)
					throw std::overflow_error("effect offense overflow");
				return p_left + p_right;
			};
			const auto multiply = [](std::int64_t p_left, std::int64_t p_right) {
				if (p_left != 0 && p_right > std::numeric_limits<std::int64_t>::max() / p_left)
					throw std::overflow_error("effect offense overflow");
				return p_left * p_right;
			};
			return add(add(multiply(p_base, 1000), multiply(strength, p_strengthRatio)), multiply(magic, p_magicRatio)) / 1000;
		}

		[[nodiscard]] std::vector<BattleUnitId> unitScope(const CastPlan &p_plan, EffectApplyTo p_applyTo)
		{
			switch (p_applyTo)
			{
			case EffectApplyTo::SourceUnit: return {p_plan.source};
			case EffectApplyTo::PrimaryUnit:
				return p_plan.primaryUnit ? std::vector<BattleUnitId>{*p_plan.primaryUnit} : std::vector<BattleUnitId>{};
			case EffectApplyTo::AffectedUnits: return p_plan.affectedUnits;
			default: return {};
			}
		}
	}

	bool EffectResolver::supports(const EffectPayload &p_payload) noexcept
	{
		return std::holds_alternative<DamageEffectSpec>(p_payload) || std::holds_alternative<HealEffectSpec>(p_payload) ||
			std::holds_alternative<ChangeResourceEffectSpec>(p_payload) ||
			std::holds_alternative<ApplyNextActivationPenaltyEffectSpec>(p_payload) ||
			std::holds_alternative<AdjustTurnBarEffectSpec>(p_payload);
	}

	bool EffectResolver::supportsAll(const AbilityDefinition &p_ability) noexcept
	{
		return std::ranges::all_of(p_ability.effects, [](const EffectApplication &p_effect) { return supports(p_effect.payload); });
	}

	void EffectResolver::_applyDamage(BattleContext &p_context, std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const DamageEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const std::int64_t raw = offenseAmount(p_context.tryUnit(p_plan.source), p_spec.base, p_spec.strengthRatioPermille, p_spec.magicPowerRatioPermille);
		const std::int64_t defense = p_spec.kind == DamageKind::Physical ? target.effectiveAttributes().armor : target.effectiveAttributes().resistance;
		const std::int64_t scale = p_context.registries().gameRules().battle.mitigationScale;
		if (raw != 0 && scale > std::numeric_limits<std::int64_t>::max() / raw)
			throw std::overflow_error("damage overflow");
		const std::int64_t mitigated = raw * scale / (defense + scale);
		if (mitigated > std::numeric_limits<int>::max()) throw std::overflow_error("damage overflow");
		const int before = target.health();
		const int applied = std::min(before, static_cast<int>(mitigated));
		target.setHealth(before - applied);

		Damage event;
		event.source = p_plan.source; event.target = p_targetId; event.abilityId = p_plan.abilityId; event.effectId = p_effect.id;
		event.kind = p_spec.kind; event.computedDamage = static_cast<int>(mitigated); event.appliedToHealth = applied;
		event.healthBefore = before; event.healthAfter = target.health();
		const BattleEventOrigin origin = makeOrigin(p_plan, p_effect, p_targetId);
		p_events.push_back({BattleEventCategory::Gameplay, origin, event});
		if (before > 0 && target.health() == 0) p_context.removeUnit(p_targetId, RemovalReason::Defeated, origin, p_events);
	}

	void EffectResolver::_applyHealing(BattleContext &p_context, std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const HealEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const int requested = static_cast<int>(offenseAmount(p_context.tryUnit(p_plan.source), p_spec.base, p_spec.strengthRatioPermille, p_spec.magicPowerRatioPermille));
		const int before = target.health();
		const int applied = std::min(requested, target.maxHealth() - before);
		target.setHealth(before + applied);
		Healing event;
		event.source = p_plan.source; event.target = p_targetId; event.abilityId = p_plan.abilityId; event.effectId = p_effect.id;
		event.requestedHealing = requested; event.appliedHealing = applied; event.healthBefore = before; event.healthAfter = target.health();
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId), event});
	}

	void EffectResolver::_applyResourceChange(BattleContext &p_context, std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const ChangeResourceEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const int before = p_spec.resource == BattleResource::ActionPoints ? target.actionPoints() : target.movementPoints();
		const int maximum = p_spec.resource == BattleResource::ActionPoints ? target.maxActionPoints() : target.maxMovementPoints();
		const int after = static_cast<int>(std::clamp(static_cast<long long>(before) + p_spec.delta, 0LL, static_cast<long long>(maximum)));
		if (p_spec.resource == BattleResource::ActionPoints) target.setActionPoints(after); else target.setMovementPoints(after);
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId),
			ResourceChanged{p_targetId, p_spec.resource, static_cast<int>(p_spec.delta), after - before, before, after, ResourceChangeReason::Effect}});
	}

	void EffectResolver::_applyNextActivationPenalty(BattleContext &p_context, std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan, const EffectApplication &p_effect, BattleUnitId p_targetId, const ApplyNextActivationPenaltyEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		const int before = p_spec.resource == BattleResource::ActionPoints ? target.nextActivationPenalty().actionPoints : target.nextActivationPenalty().movementPoints;
		target.addNextActivationPenalty(p_spec.resource, static_cast<int>(p_spec.amount));
		p_events.push_back({BattleEventCategory::Gameplay, makeOrigin(p_plan, p_effect, p_targetId),
			NextActivationPenaltyApplied{p_targetId, p_spec.resource, static_cast<int>(p_spec.amount), static_cast<int>(p_spec.amount), before, before + static_cast<int>(p_spec.amount)}});
	}

	void EffectResolver::_applyTurnBarAdjustment(BattleContext &p_context, const CastPlan &p_plan,
		BattleUnitId p_targetId, const AdjustTurnBarEffectSpec &p_spec)
	{
		BattleUnit &target = p_context.unitMutable(p_targetId);
		target.setTurnBarFill(clamp(target.turnBarFill() + p_spec.delta, BattleTime{}, target.effectiveAttributes().stamina));
	}

	void EffectResolver::resolveAbility(BattleContext &p_context, std::vector<StagedEvent> &p_events,
		const CastPlan &p_plan, const AbilityDefinition &p_ability)
	{
		for (const EffectApplication &effect : p_ability.effects)
		{
			if (effect.requiresLivingSource && !isActive(p_context, p_plan.source)) continue;
			for (const BattleUnitId target : unitScope(p_plan, effect.applyTo))
			{
				if (!isActive(p_context, target)) continue;
				std::visit([&](const auto &payload) {
					using Payload = std::decay_t<decltype(payload)>;
					if constexpr (std::is_same_v<Payload, DamageEffectSpec>) _applyDamage(p_context, p_events, p_plan, effect, target, payload);
					else if constexpr (std::is_same_v<Payload, HealEffectSpec>) _applyHealing(p_context, p_events, p_plan, effect, target, payload);
					else if constexpr (std::is_same_v<Payload, ChangeResourceEffectSpec>) _applyResourceChange(p_context, p_events, p_plan, effect, target, payload);
					else if constexpr (std::is_same_v<Payload, ApplyNextActivationPenaltyEffectSpec>) _applyNextActivationPenalty(p_context, p_events, p_plan, effect, target, payload);
					else if constexpr (std::is_same_v<Payload, AdjustTurnBarEffectSpec>) _applyTurnBarAdjustment(p_context, p_plan, target, payload);
				}, effect.payload);
			}
		}
	}
}
