#include "battle/rules/battle_action_resolver.hpp"

#include "abilities/ability.hpp"
#include "abilities/effect.hpp"
#include "abilities/effects/effect_types.hpp"
#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "battle/rules/battle_resource_rules.hpp"
#include "battle/rules/battle_status_rules.hpp"
#include "battle/rules/battle_unit_rules.hpp"
#include "statuses/status.hpp"

#include <cmath>
#include <memory>

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
			p_context.report(ResourceConsumedEvent{.context = {.turnIndex = turn, .caster = &source}, .resource = kind, .amount = amount});
		};
		if (p_action.kind() == BattleActionKind::EndTurn)
		{
			return true;
		}
		if (p_action.kind() == BattleActionKind::Move)
		{
			const auto &move = static_cast<const MoveAction &>(p_action);
			BattleStatusRules::applyHook(source, StatusHookPoint::BeforeConsumingResources, move.distance);
			consume(BattleResourceKind::MP, move.distance);
			BattleStatusRules::applyHook(source, StatusHookPoint::AfterConsumingResources, move.distance);
			if (!p_context.tryMoveUnit(source, move.destination))
			{
				return false;
			}
			p_context.currentTurn.moved = true;
			p_context.report(DistanceTravelledEvent{.context = {.turnIndex = turn, .caster = &source}, .distance = move.distance});
			BattleUnitRules::resolvePendingDefeats(p_context, &source);
			return true;
		}

		const auto &action = static_cast<const AbilityAction &>(p_action);
		const int totalCost = action.apCost() + action.mpCost();
		if (totalCost > 0)
		{
			BattleStatusRules::applyHook(source, StatusHookPoint::BeforeConsumingResources, totalCost);
		}
		consume(BattleResourceKind::AP, action.apCost());
		consume(BattleResourceKind::MP, action.mpCost());
		if (totalCost > 0)
		{
			BattleStatusRules::applyHook(source, StatusHookPoint::AfterConsumingResources, totalCost);
		}
		const int distance = source.boardPosition ? std::abs(source.boardPosition->x - action.targetCells.front().x) +
														std::abs(source.boardPosition->z - action.targetCells.front().z)
												  : 0;
		p_context.report(AbilityCastEvent{.context = {.turnIndex = turn, .sourceAbility = &action.ability, .caster = &source}, .distance = distance});

		std::vector<std::shared_ptr<const Effect>> legacy;
		const auto *effects = &action.ability.effects;
		if (effects->empty())
		{
			auto damage = std::make_shared<DamageEffect>();
			damage->baseDamage = action.ability.baseDamage;
			damage->damageKind = action.ability.damageKind;
			damage->attackRatio = action.ability.attackRatio;
			damage->magicRatio = action.ability.magicRatio;
			legacy.push_back(std::move(damage));
			effects = &legacy;
		}

		for (const spk::Vector3Int &anchor : action.targetCells)
		{
			for (const spk::Vector3Int &cell : BattleActionValidator::getAreaCells(p_context, source, action.ability, anchor))
			{
				BattleAbilityExecutionContext execution{
					.context = p_context, .ability = &action.ability, .sourceObject = &source, .targetObject = p_context.tryGetUnitAtIncludingDefeated(cell), .anchorCell = anchor, .affectedCell = cell, .mitigationScaling = p_scaling};
				for (const auto &effect : *effects)
				{
					effect->apply(execution);
				}
			}
		}

		BattleUnitRules::resolvePendingDefeats(p_context, &source, &action.ability);
		p_context.currentTurn.castAbility = true;
		return true;
	}
}
