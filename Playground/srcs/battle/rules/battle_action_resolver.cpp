#include "battle/rules/battle_action_resolver.hpp"

#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/math_formulas.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "battle/rules/battle_resource_rules.hpp"

#include <cmath>

namespace pg
{
	bool BattleActionResolver::resolve(BattleContext &p_context, const BattleAction &p_action, float p_scaling)
	{
		if (!BattleActionValidator::validate(p_context, p_action))
		{
			return false;
		}
		BattleUnit &source = p_action.source;
		const int turn = p_context.currentTurn.turnIndex;
		auto consume = [&](BattleResourceKind kind, int amount) {
			if (amount <= 0)
			{
				return;
			}
			(void)BattleResourceRules::change(source, kind, -amount);
			p_context.report({.type = BattleEventType::ResourceConsumed, .turnIndex = turn, .caster = &source, .amount = amount, .resource = kind});
		};
		if (p_action.kind() == BattleActionKind::EndTurn)
		{
			return true;
		}
		if (p_action.kind() == BattleActionKind::Move)
		{
			const auto &move = static_cast<const MoveAction &>(p_action);
			consume(BattleResourceKind::MP, move.distance);
			if (!p_context.tryMoveUnit(source, move.destination))
			{
				return false;
			}
			p_context.currentTurn.moved = true;
			p_context.report({.type = BattleEventType::DistanceTravelled, .turnIndex = turn, .caster = &source, .distance = move.distance});
			return true;
		}
		const auto &action = static_cast<const AbilityAction &>(p_action);
		consume(BattleResourceKind::AP, action.apCost());
		consume(BattleResourceKind::MP, action.mpCost());
		const int distance = source.boardPosition ? std::abs(source.boardPosition->x - action.targetCells.front().x) + std::abs(source.boardPosition->z - action.targetCells.front().z) : 0;
		p_context.report({.type = BattleEventType::AbilityCast, .turnIndex = turn, .sourceAbility = &action.ability, .caster = &source, .distance = distance});
		for (const auto &cell : action.targetCells)
		{
			auto *target = dynamic_cast<BattleUnit *>(p_context.board.tryGetUnitAt(cell));
			if (!target)
			{
				continue;
			}
			const int computed = MathFormulas::computeDamage(source.attributes, target->attributes, action.ability, p_scaling);
			auto absorption = target->attributes.absorbDamage(action.ability.damageKind, computed);
			if (absorption.absorbed > 0)
			{
				p_context.report({.type = BattleEventType::DamageAbsorbed, .turnIndex = turn, .sourceAbility = &action.ability, .caster = &source, .target = target, .amount = absorption.absorbed});
			}
			const auto change = BattleResourceRules::change(*target, BattleResourceKind::Health, -absorption.remaining);
			p_context.report({.type = BattleEventType::Damage, .turnIndex = turn, .sourceAbility = &action.ability, .caster = &source, .target = target, .amount = change.lost});
			if (target->isDefeated())
			{
				(void)p_context.defeatUnit(*target);
				p_context.report({.type = BattleEventType::UnitDefeated, .turnIndex = turn, .sourceAbility = &action.ability, .caster = &source, .target = target});
			}
			else
			{
				p_context.report({.type = BattleEventType::HitSurvived, .turnIndex = turn, .sourceAbility = &action.ability, .caster = &source, .target = target, .amount = target->attributes.hp.current()});
			}
		}
		p_context.currentTurn.castAbility = true;
		return true;
	}
}
