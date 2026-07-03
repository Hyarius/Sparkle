#include "battle/rules/battle_status_rules.hpp"

#include "abilities/effect.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "statuses/status.hpp"

#include <vector>

namespace
{
	thread_local bool applyingHook = false;

	struct HookGuard
	{
		HookGuard()
		{
			applyingHook = true;
		}
		~HookGuard()
		{
			applyingHook = false;
		}
	};
}

namespace pg
{
	void BattleStatusRules::applyHook(BattleUnit &p_owner, StatusHookPoint p_hookPoint, int p_triggerAmount)
	{
		BattleContext *context = p_owner.statuses.context();
		if (context == nullptr || applyingHook)
		{
			return;
		}

		struct PendingStatus
		{
			const Status *definition = nullptr;
			int stacks = 0;
		};
		std::vector<PendingStatus> pending;
		for (const BattleStatus &entry : p_owner.statuses.entries())
		{
			if (entry.definition != nullptr && entry.definition->hookPoint == p_hookPoint)
			{
				pending.push_back({entry.definition, entry.stacks});
			}
		}

		HookGuard guard;
		const spk::Vector3Int cell = p_owner.boardPosition.value_or(
			p_owner.lastBoardPosition.value_or(spk::Vector3Int{}));
		for (const PendingStatus &status : pending)
		{
			for (int stack = 0; stack < status.stacks; ++stack)
			{
				BattleAbilityExecutionContext execution{
					.context = *context,
					.sourceObject = &p_owner,
					.targetObject = &p_owner,
					.anchorCell = cell,
					.affectedCell = cell,
					.triggerAmount = p_triggerAmount};
				for (const auto &effect : status.definition->effects)
				{
					effect->apply(execution);
				}
			}
		}
	}

	bool BattleStatusRules::isApplyingHook() noexcept
	{
		return applyingHook;
	}

	void BattleStatusRules::advanceSeconds(BattleContext &p_context, float p_seconds)
	{
		for (BattleUnit *unit : p_context.allUnits())
		{
			for (const BattleStatusRemoval &removed : unit->statuses.advanceSeconds(p_seconds))
			{
				p_context.report({.type = BattleEventType::StatusRemoved, .turnIndex = p_context.currentTurn.turnIndex, .status = removed.definition, .caster = unit, .target = unit, .amount = removed.stacks});
			}
		}
	}
}
