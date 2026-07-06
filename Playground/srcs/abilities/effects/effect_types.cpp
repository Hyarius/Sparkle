#include "abilities/effects/effect_types.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "battle/math_formulas.hpp"
#include "battle/rules/battle_resource_rules.hpp"
#include "statuses/status.hpp"

#include <algorithm>
#include <cmath>

namespace
{
	using namespace pg;

	BattleUnit *unit(BattleObject *p_object)
	{
		return dynamic_cast<BattleUnit *>(p_object);
	}

	BattleEventContext eventContext(const BattleAbilityExecutionContext &p_context, BattleUnit *p_target = nullptr)
	{
		return {.turnIndex = p_context.context.currentTurn.turnIndex, .sourceAbility = p_context.ability, .caster = unit(p_context.sourceObject), .target = p_target};
	}

	BattleResourceKind eventResource(EffectResource p_resource)
	{
		switch (p_resource)
		{
		case EffectResource::Health:
			return BattleResourceKind::Health;
		case EffectResource::AP:
			return BattleResourceKind::AP;
		case EffectResource::MP:
			return BattleResourceKind::MP;
		case EffectResource::Range:
			return BattleResourceKind::Range;
		case EffectResource::TurnBarTime:
			return BattleResourceKind::TurnBarTime;
		}
		return BattleResourceKind::None;
	}

	int changeIntegerResource(BattleUnit &p_unit, EffectResource p_resource, int p_delta)
	{
		if (p_resource == EffectResource::Range)
		{
			const int before = p_unit.attributes.bonusRange;
			p_unit.attributes.bonusRange = std::max(0, before + p_delta);
			return static_cast<int>(p_unit.attributes.bonusRange) - before;
		}
		BattleResourceKind resource = eventResource(p_resource);
		const auto changed = BattleResourceRules::change(p_unit, resource, p_delta);
		return changed.gained - changed.lost;
	}
}

namespace pg
{
	void DamageEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *caster = unit(p_context.sourceObject);
		BattleUnit *target = unit(p_context.targetObject);
		if (caster == nullptr || target == nullptr || target->isDefeated())
		{
			return;
		}
		const float raw = std::max(0.0f, static_cast<float>(baseDamage) + static_cast<int>(caster->attributes.attack) * attackRatio + static_cast<int>(caster->attributes.magic) * magicRatio);
		const int defense = damageKind == DamageKind::Physical
								? std::max(0, static_cast<int>(target->attributes.armor) - static_cast<int>(caster->attributes.armorPenetration))
								: std::max(0, static_cast<int>(target->attributes.resistance) - static_cast<int>(caster->attributes.resistancePenetration));
		const int incoming = std::max(0, static_cast<int>(std::lround(raw * (1.0f - MathFormulas::mitigationRatio(defense, p_context.mitigationScaling)))));
		const AbsorptionResult absorption = target->attributes.absorbDamage(damageKind, incoming);
		if (absorption.absorbed > 0)
		{
			p_context.context.report(DamageAbsorbedEvent{.context = eventContext(p_context, target), .amount = absorption.absorbed, .kind = damageKind});
		}
		for (ShieldKind broken : absorption.broken)
		{
			p_context.context.report(ShieldBrokenEvent{.context = eventContext(p_context, target), .kind = broken});
		}
		const auto change = BattleResourceRules::change(*target, BattleResourceKind::Health, -absorption.remaining);
		p_context.context.report(DamageEvent{.context = eventContext(p_context, target), .amount = change.lost, .kind = damageKind});
		const float ratio = damageKind == DamageKind::Physical ? static_cast<float>(caster->attributes.lifeSteal)
															   : static_cast<float>(caster->attributes.omnivampirism);
		if (change.lost > 0 && ratio > 0.0f)
		{
			const auto healed = BattleResourceRules::change(*caster, BattleResourceKind::Health, static_cast<int>(std::lround(change.lost * ratio)));
			if (healed.gained > 0)
			{
				p_context.context.report(HealEvent{.context = eventContext(p_context, caster), .amount = healed.gained});
			}
		}
		if (!target->isDefeated())
		{
			p_context.context.report(HitSurvivedEvent{.context = eventContext(p_context, target), .remainingHealth = target->attributes.hp.current()});
		}
	}

	void HealEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *caster = unit(p_context.sourceObject);
		BattleUnit *target = unit(p_context.targetObject);
		if (caster == nullptr || target == nullptr || target->isDefeated())
		{
			return;
		}
		const float raw = std::max(0.0f, static_cast<float>(baseHealing) + static_cast<int>(caster->attributes.attack) * attackRatio + static_cast<int>(caster->attributes.magic) * magicRatio);
		const int amount = std::max(0, static_cast<int>(std::lround(raw * (1.0f + static_cast<float>(caster->attributes.bonusHealing)))));
		const auto changed = BattleResourceRules::change(*target, BattleResourceKind::Health, amount);
		p_context.context.report(HealEvent{.context = eventContext(p_context, target), .amount = changed.gained});
	}

	void ApplyStatusEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target != nullptr && statusDefinition != nullptr)
		{
			const BattleStatusRemoval applied = target->statuses.add(*statusDefinition, stackCount, duration);
			if (applied.stacks > 0)
			{
				p_context.context.report(StatusAppliedEvent{.context = eventContext(p_context, target), .status = statusDefinition, .count = applied.stacks});
			}
		}
	}
	void RemoveStatusEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		if (BattleUnit *target = unit(p_context.targetObject))
		{
			const BattleStatusRemoval removed = target->statuses.remove(status, stackCount);
			if (removed.stacks > 0)
			{
				p_context.context.report(StatusRemovedEvent{.context = eventContext(p_context, target), .status = removed.definition, .count = removed.stacks});
			}
		}
	}
	void ConsumeStatusEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		if (BattleUnit *target = unit(p_context.targetObject))
		{
			const BattleStatusRemoval removed = target->statuses.consume(status, stackCount);
			if (removed.stacks > 0)
			{
				p_context.context.report(StatusRemovedEvent{.context = eventContext(p_context, target), .status = removed.definition, .count = removed.stacks});
			}
		}
	}
	void CleanseEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		if (BattleUnit *target = unit(p_context.targetObject))
		{
			for (const BattleStatusRemoval &removed : target->statuses.removeByTags(tags))
			{
				p_context.context.report(StatusRemovedEvent{.context = eventContext(p_context, target), .status = removed.definition, .count = removed.stacks});
			}
		}
	}

	void ReviveEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target == nullptr || !target->isDefeated())
		{
			return;
		}
		const auto changed = BattleResourceRules::change(*target, BattleResourceKind::Health, std::max(1, healthRestored));
		if (changed.gained == 0)
		{
			return;
		}
		if (!target->boardPosition && target->lastBoardPosition)
		{
			(void)p_context.context.tryPlaceUnit(*target, *target->lastBoardPosition);
		}
		p_context.context.report(ReviveEvent{.context = eventContext(p_context, target), .amount = changed.gained});
	}

	void ApplyShieldEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target == nullptr || target->isDefeated())
		{
			return;
		}
		target->attributes.addShield(kind, amount, durationTurns);
		p_context.context.report(ShieldAppliedEvent{.context = eventContext(p_context, target), .amount = amount, .kind = kind});
	}

	void ResourceChangeEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target == nullptr)
		{
			return;
		}
		int actual = 0;
		if (resource == EffectResource::TurnBarTime)
		{
			const float before = target->attributes.turnBar.current();
			target->attributes.turnBar.change(static_cast<float>(value));
			actual = static_cast<int>(std::lround(target->attributes.turnBar.current() - before));
		}
		else
		{
			actual = changeIntegerResource(*target, resource, value);
		}
		p_context.context.report(ResourceChangedEvent{.context = eventContext(p_context, target), .resource = eventResource(resource), .amount = std::abs(actual), .delta = static_cast<float>(actual)});
	}

	void StealResourceEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *caster = unit(p_context.sourceObject);
		BattleUnit *target = unit(p_context.targetObject);
		if (caster == nullptr || target == nullptr || caster == target)
		{
			return;
		}
		int removed = 0;
		if (resource == EffectResource::TurnBarTime)
		{
			const float before = target->attributes.turnBar.current();
			target->attributes.turnBar.change(-static_cast<float>(value));
			removed = static_cast<int>(std::lround(before - target->attributes.turnBar.current()));
			caster->attributes.turnBar.change(static_cast<float>(removed));
		}
		else
		{
			removed = std::max(0, -changeIntegerResource(*target, resource, -value));
			(void)changeIntegerResource(*caster, resource, removed);
		}
		if (removed > 0)
		{
			p_context.context.report(ResourceStolenEvent{.context = eventContext(p_context, target), .resource = eventResource(resource), .amount = removed});
		}
	}

	void DisplacementEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *caster = unit(p_context.sourceObject);
		BattleUnit *target = unit(p_context.targetObject);
		if (caster == nullptr || target == nullptr || caster == target || !caster->boardPosition || !target->boardPosition)
		{
			return;
		}
		int dx = target->boardPosition->x - caster->boardPosition->x, dz = target->boardPosition->z - caster->boardPosition->z;
		spk::Vector3Int step{};
		if (std::abs(dx) >= std::abs(dz))
		{
			step.x = (dx > 0) - (dx < 0);
		}
		else
		{
			step.z = (dz > 0) - (dz < 0);
		}
		if (orientation == DisplacementOrientation::TowardCaster)
		{
			step.x = -step.x;
			step.z = -step.z;
		}
		int moved = 0;
		for (; moved < force; ++moved)
		{
			const spk::Vector3Int next = *target->boardPosition + step;
			if (!p_context.context.tryMoveUnit(*target, next))
			{
				break;
			}
		}
		if (moved > 0)
		{
			p_context.context.report(DisplacementEvent{.context = eventContext(p_context, target), .distance = moved, .orientation = orientation});
		}
	}

	void SwapPositionEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		BattleUnit *anchor = dynamic_cast<BattleUnit *>(p_context.context.board.tryGetUnitAt(p_context.anchorCell));
		if (target != nullptr && anchor != nullptr && target != anchor && p_context.context.trySwapUnits(*target, *anchor))
		{
			p_context.context.report(SwapPositionEvent{.context = eventContext(p_context, target)});
		}
	}

	void SwapPositionWithCasterEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *caster = unit(p_context.sourceObject);
		BattleUnit *target = unit(p_context.targetObject);
		if (caster != nullptr && target != nullptr && caster != target && p_context.context.trySwapUnits(*caster, *target))
		{
			p_context.context.report(SwapPositionEvent{.context = eventContext(p_context, target)});
		}
	}

	void TeleportSelfEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		if (p_context.affectedCell != p_context.anchorCell)
		{
			return;
		}
		BattleUnit *caster = unit(p_context.sourceObject);
		if (caster == nullptr || !caster->boardPosition)
		{
			return;
		}
		const int distance = std::abs(caster->boardPosition->x - p_context.anchorCell.x) + std::abs(caster->boardPosition->z - p_context.anchorCell.z);
		if (distance > 0 && p_context.context.tryMoveUnit(*caster, p_context.anchorCell))
		{
			p_context.context.report(TeleportedEvent{.context = eventContext(p_context, caster), .distance = distance});
		}
	}

	void AdjustTurnBarTimeEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target == nullptr)
		{
			return;
		}
		const float resistance = std::max(0.0f, static_cast<float>(target->attributes.timeEffectResistance));
		const float scaling = std::max(0.001f, p_context.timeEffectResistanceScaling);
		const float actualDelta = delta * (1.0f - resistance / (resistance + scaling));
		const float before = target->attributes.turnBar.current();
		target->attributes.turnBar.change(actualDelta);
		p_context.context.report(TurnBarTimeAdjustedEvent{.context = eventContext(p_context, target), .delta = target->attributes.turnBar.current() - before});
	}

	void AdjustTurnBarDurationEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target == nullptr)
		{
			return;
		}
		const float before = target->attributes.turnBar.max();
		target->attributes.turnBar.setMax(std::max(p_context.minimumTurnBarDuration, before + delta));
		p_context.context.report(TurnBarDurationAdjustedEvent{.context = eventContext(p_context, target), .delta = target->attributes.turnBar.max() - before});
	}

	void PlaceObjectEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		const int turns = duration.kind == DurationKind::TurnBased ? duration.turns : -1;
		(void)p_context.context.placeObject(object, p_context.affectedCell, turns);
	}

	void RemoveObjectEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		(void)p_context.context.removeObjects(p_context.affectedCell, tags);
	}
}
