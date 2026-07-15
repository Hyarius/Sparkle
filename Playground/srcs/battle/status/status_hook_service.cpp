#include "battle/status/status_hook_service.hpp"

#include "battle/effects/effect_resolver.hpp"
#include "core/registries.hpp"

#include <algorithm>
#include <ranges>

namespace pg
{
	namespace
	{
		class ReactionScope
		{
		private:
			BattleContext *_context = nullptr;

		public:
			ReactionScope(BattleContext &p_context, ReactionKind p_kind) :
				_context(p_context.beginReaction(p_kind) ? &p_context : nullptr)
			{
			}
			~ReactionScope()
			{
				if (_context != nullptr)
				{
					_context->endReaction();
				}
			}
			[[nodiscard]] explicit operator bool() const noexcept
			{
				return _context != nullptr;
			}
		};

		[[nodiscard]] CastPlan frameFor(
			const BattleContext &p_context,
			BattleUnitId p_source,
			BattleUnitId p_primary,
			std::string p_ownerId)
		{
			const BattleUnit &owner = p_context.unit(p_primary);
			const BoardCell anchor = p_context.board().occupancy().cellOf(p_primary).value_or(owner.lastOccupiedCell().value_or(BoardCell{}));
			return {
				.source = p_source,
				.abilityId = std::move(p_ownerId),
				.sourceCellAtCapture = p_context.board().occupancy().cellOf(p_source).value_or(anchor),
				.anchor = anchor,
				.primaryUnit = p_primary,
				.areaCells = {anchor},
				.affectedCells = {anchor},
				.affectedUnits = {p_primary}};
		}

		[[nodiscard]] bool triggerPermits(const BattleObjectInstance &p_object, const TargetFilter &p_filter, const BattleUnit &p_target)
		{
			if (!p_target.isActiveCombatant() && !p_filter.allowDefeated)
			{
				return false;
			}
			if (p_object.creator.has_value() && *p_object.creator == p_target.id())
			{
				return p_filter.allowSelf;
			}
			return p_object.creatorSide == p_target.side() ? p_filter.allowAllies : p_filter.allowEnemies;
		}
	}

	void StatusHookService::dispatchStatusHook(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		BattleUnitId p_owner,
		StatusHook p_hook,
		std::optional<BattleUnitId> p_primary,
		std::optional<BattleStatusInstanceId> p_onlyInstance)
	{
		if (!p_context.unit(p_owner).isActiveCombatant())
		{
			return;
		}
		ReactionScope scope(p_context, ReactionKind::StatusHook);
		if (!scope)
		{
			return;
		}

		std::vector<BattleStatusInstanceId> ids;
		for (const BattleStatusInstance &instance : p_context.unit(p_owner).passiveStatuses())
		{
			ids.push_back(instance.id);
		}
		for (const BattleStatusInstance &instance : p_context.unit(p_owner).transientStatuses())
		{
			ids.push_back(instance.id);
		}
		for (const BattleStatusInstanceId id : ids)
		{
			if (p_onlyInstance.has_value() && id != *p_onlyInstance)
			{
				continue;
			}
			const BattleUnit &owner = p_context.unit(p_owner);
			const auto locate = [id](const std::vector<BattleStatusInstance> &instances) {
				return std::ranges::find_if(instances, [id](const BattleStatusInstance &instance) {
					return instance.id == id;
				});
			};
			auto passive = locate(owner.passiveStatuses());
			auto transient = locate(owner.transientStatuses());
			if (passive == owner.passiveStatuses().end() && transient == owner.transientStatuses().end())
			{
				continue;
			}
			const BattleStatusInstance &instance = passive != owner.passiveStatuses().end() ? *passive : *transient;
			const StatusDefinition *definition = p_context.registries().statuses().tryGet(instance.definitionId);
			if (definition == nullptr)
			{
				continue;
			}
			const auto hook = std::ranges::find_if(definition->hooks, [p_hook](const StatusHookSpec &spec) {
				return spec.hook == p_hook;
			});
			if (hook == definition->hooks.end())
			{
				continue;
			}
			const BattleUnitId source = instance.appliedBy.value_or(p_owner);
			const BattleUnitId primary = p_primary.value_or(p_owner);
			AbilityDefinition synthetic;
			synthetic.id = "status:" + definition->id + ":" + hook->id;
			synthetic.effects = hook->effects;
			EffectResolver::resolveAbility(p_context, p_events, frameFor(p_context, source, primary, synthetic.id), synthetic);
		}
	}

	void StatusHookService::dispatchObjectTrigger(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		BattleObjectTrigger p_trigger,
		BattleUnitId p_unit,
		BoardCell p_cell)
	{
		if (!p_context.unit(p_unit).isActiveCombatant())
		{
			return;
		}
		ReactionScope scope(p_context, ReactionKind::BattleObjectTrigger);
		if (!scope)
		{
			return;
		}
		const std::vector<BattleObjectId> ids(p_context.board().occupancy().objectsAt(p_cell).begin(), p_context.board().occupancy().objectsAt(p_cell).end());
		for (const BattleObjectId id : ids)
		{
			BattleObjectInstance *live = p_context.objectMutable(id);
			if (live == nullptr)
			{
				continue;
			}
			const BattleObjectDefinition *definition = p_context.registries().battleObjects().tryGet(live->definitionId);
			if (definition == nullptr)
			{
				continue;
			}
			for (std::size_t index = 0; index < definition->triggers.size(); ++index)
			{
				live = p_context.objectMutable(id);
				if (live == nullptr || index >= live->triggerStates.size())
				{
					break;
				}
				const BattleObjectTriggerSpec &trigger = definition->triggers[index];
				BattleObjectTriggerState &state = live->triggerStates[index];
				if (trigger.trigger != p_trigger || (trigger.maxTriggers != 0 && state.timesTriggered >= trigger.maxTriggers) ||
					!triggerPermits(*live, trigger.targetFilter, p_context.unit(p_unit)))
				{
					continue;
				}
				++state.timesTriggered;
				const std::uint32_t triggerCount = state.timesTriggered;
				const bool exhausts = trigger.removeWhenExhausted && trigger.maxTriggers != 0 && triggerCount >= trigger.maxTriggers;
				BattleEventOrigin origin{.sourceUnit = live->creator, .targetUnit = p_unit, .statusId = std::nullopt, .objectId = id};
				p_events.push_back({BattleEventCategory::Gameplay, origin, BattleObjectTriggered{id, live->definitionId, p_unit, p_cell, trigger.id, static_cast<int>(triggerCount)}});
				AbilityDefinition synthetic;
				synthetic.id = "object:" + live->definitionId + ":" + trigger.id;
				synthetic.effects = trigger.effects;
				const BattleUnitId source = live->creator.value_or(p_unit);
				EffectResolver::resolveAbility(p_context, p_events, frameFor(p_context, source, p_unit, synthetic.id), synthetic);
				live = p_context.objectMutable(id);
				if (live != nullptr && exhausts)
				{
					const BattleObjectInstance copy = *live;
					(void)p_context.removeObject(id);
					p_events.push_back({BattleEventCategory::Gameplay, origin, BattleObjectRemoved{id, copy.definitionId, copy.cell, BattleObjectRemovalReason::TriggerExhausted}});
					break;
				}
			}
		}
	}

	void StatusHookService::dispatchRemovedHook(
		BattleContext &p_context,
		std::vector<StagedEvent> &p_events,
		BattleUnitId p_owner,
		const BattleStatusInstance &p_removed)
	{
		if (!p_context.unit(p_owner).isActiveCombatant())
		{
			return;
		}
		ReactionScope scope(p_context, ReactionKind::StatusHook);
		if (!scope)
		{
			return;
		}
		const StatusDefinition *definition = p_context.registries().statuses().tryGet(p_removed.definitionId);
		if (definition == nullptr)
		{
			return;
		}
		const auto hook = std::ranges::find_if(definition->hooks, [](const StatusHookSpec &spec) {
			return spec.hook == StatusHook::Removed;
		});
		if (hook == definition->hooks.end())
		{
			return;
		}
		const BattleUnitId source = p_removed.appliedBy.value_or(p_owner);
		AbilityDefinition synthetic;
		synthetic.id = "status:" + definition->id + ":" + hook->id;
		synthetic.effects = hook->effects;
		EffectResolver::resolveAbility(p_context, p_events, frameFor(p_context, source, p_owner, synthetic.id), synthetic);
	}
}
