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

	BattleEvent event(const BattleAbilityExecutionContext &p_context, BattleEventType p_type, BattleUnit *p_target = nullptr)
	{
		return {.type = p_type, .turnIndex = p_context.context.currentTurn.turnIndex, .sourceAbility = p_context.ability, .caster = unit(p_context.sourceObject), .target = p_target};
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
			auto reported = event(p_context, BattleEventType::DamageAbsorbed, target);
			reported.amount = absorption.absorbed;
			reported.damageKind = damageKind;
			p_context.context.report(reported);
		}
		for (ShieldKind broken : absorption.broken)
		{
			auto reported = event(p_context, BattleEventType::ShieldBroken, target);
			reported.amount = static_cast<int>(broken);
			reported.shieldKind = broken;
			p_context.context.report(reported);
		}
		const auto change = BattleResourceRules::change(*target, BattleResourceKind::Health, -absorption.remaining);
		auto damage = event(p_context, BattleEventType::Damage, target);
		damage.amount = change.lost;
		damage.damageKind = damageKind;
		p_context.context.report(damage);
		const float ratio = damageKind == DamageKind::Physical ? static_cast<float>(caster->attributes.lifeSteal)
															   : static_cast<float>(caster->attributes.omnivampirism);
		if (change.lost > 0 && ratio > 0.0f)
		{
			const auto healed = BattleResourceRules::change(*caster, BattleResourceKind::Health, static_cast<int>(std::lround(change.lost * ratio)));
			if (healed.gained > 0)
			{
				auto report = event(p_context, BattleEventType::Heal, caster);
				report.amount = healed.gained;
				p_context.context.report(report);
			}
		}
		if (!target->isDefeated())
		{
			auto survived = event(p_context, BattleEventType::HitSurvived, target);
			survived.amount = target->attributes.hp.current();
			p_context.context.report(survived);
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
		auto report = event(p_context, BattleEventType::Heal, target);
		report.amount = changed.gained;
		p_context.context.report(report);
	}

	void ApplyStatusEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target != nullptr && statusDefinition != nullptr)
		{
			const BattleStatusRemoval applied = target->statuses.add(*statusDefinition, stackCount, duration);
			if (applied.stacks > 0)
			{
				auto report = event(p_context, BattleEventType::StatusApplied, target);
				report.status = statusDefinition;
				report.amount = applied.stacks;
				p_context.context.report(report);
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
				auto report = event(p_context, BattleEventType::StatusRemoved, target);
				report.status = removed.definition;
				report.amount = removed.stacks;
				p_context.context.report(report);
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
				auto report = event(p_context, BattleEventType::StatusRemoved, target);
				report.status = removed.definition;
				report.amount = removed.stacks;
				p_context.context.report(report);
			}
		}
	}
	void CleanseEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		if (BattleUnit *target = unit(p_context.targetObject))
		{
			for (const BattleStatusRemoval &removed : target->statuses.removeByTags(tags))
			{
				auto report = event(p_context, BattleEventType::StatusRemoved, target);
				report.status = removed.definition;
				report.amount = removed.stacks;
				p_context.context.report(report);
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
		auto report = event(p_context, BattleEventType::Revive, target);
		report.amount = changed.gained;
		p_context.context.report(report);
	}

	void ApplyShieldEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		if (target == nullptr || target->isDefeated())
		{
			return;
		}
		target->attributes.addShield(kind, amount, durationTurns);
		auto report = event(p_context, BattleEventType::ShieldApplied, target);
		report.amount = amount;
		report.shieldKind = kind;
		p_context.context.report(report);
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
		auto report = event(p_context, BattleEventType::ResourceChanged, target);
		report.amount = std::abs(actual);
		report.value = static_cast<float>(actual);
		report.resource = eventResource(resource);
		p_context.context.report(report);
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
			auto report = event(p_context, BattleEventType::ResourceStolen, target);
			report.amount = removed;
			report.resource = eventResource(resource);
			p_context.context.report(report);
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
			auto report = event(p_context, BattleEventType::Displacement, target);
			report.distance = moved;
			report.orientation = orientation;
			p_context.context.report(report);
		}
	}

	void SwapPositionEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *target = unit(p_context.targetObject);
		BattleUnit *anchor = dynamic_cast<BattleUnit *>(p_context.context.board.tryGetUnitAt(p_context.anchorCell));
		if (target != nullptr && anchor != nullptr && target != anchor && p_context.context.trySwapUnits(*target, *anchor))
		{
			auto report = event(p_context, BattleEventType::SwapPosition, target);
			p_context.context.report(report);
		}
	}

	void SwapPositionWithCasterEffect::apply(BattleAbilityExecutionContext &p_context) const
	{
		BattleUnit *caster = unit(p_context.sourceObject);
		BattleUnit *target = unit(p_context.targetObject);
		if (caster != nullptr && target != nullptr && caster != target && p_context.context.trySwapUnits(*caster, *target))
		{
			auto report = event(p_context, BattleEventType::SwapPosition, target);
			p_context.context.report(report);
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
			auto report = event(p_context, BattleEventType::Teleported, caster);
			report.distance = distance;
			p_context.context.report(report);
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
		auto report = event(p_context, BattleEventType::TurnBarTimeAdjusted, target);
		report.value = target->attributes.turnBar.current() - before;
		p_context.context.report(report);
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
		auto report = event(p_context, BattleEventType::TurnBarDurationAdjusted, target);
		report.value = target->attributes.turnBar.max() - before;
		p_context.context.report(report);
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
