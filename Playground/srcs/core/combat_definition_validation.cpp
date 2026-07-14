#include "core/combat_definition_validation.hpp"

#include <string>
#include <variant>

namespace pg
{
	namespace
	{
		// The effect kept its authored file and JSON path through phase 1 precisely so this
		// message can point at the line the author has to fix, rather than at the registry.
		[[noreturn]] void fail(
			const std::string &p_owner,
			const EffectApplication &p_effect,
			const std::string &p_message)
		{
			throw JsonError(
				p_effect.source.file,
				p_effect.source.jsonPath,
				"definition '" + p_owner + "' effect '" + p_effect.id + "' " + p_message);
		}

		[[nodiscard]] const StatusDefinition &requireStatus(
			const Registry<StatusDefinition> &p_statuses,
			const std::string &p_owner,
			const EffectApplication &p_effect,
			const std::string &p_statusId)
		{
			const StatusDefinition *status = p_statuses.tryGet(p_statusId);
			if (status == nullptr)
			{
				fail(p_owner, p_effect, "references unknown status '" + p_statusId + "'");
			}
			return *status;
		}

		void requireBattleObject(
			const Registry<BattleObjectDefinition> &p_battleObjects,
			const std::string &p_owner,
			const EffectApplication &p_effect,
			const std::string &p_objectId)
		{
			if (!p_battleObjects.contains(p_objectId))
			{
				fail(p_owner, p_effect, "references unknown battle object '" + p_objectId + "'");
			}
		}

		void validateEffect(
			const Registry<StatusDefinition> &p_statuses,
			const Registry<BattleObjectDefinition> &p_battleObjects,
			const std::string &p_owner,
			const EffectApplication &p_effect)
		{
			if (const auto *applyStatus = std::get_if<ApplyStatusEffectSpec>(&p_effect.payload))
			{
				const StatusDefinition &status =
					requireStatus(p_statuses, p_owner, p_effect, applyStatus->status);

				// A stunned unit cannot reliably consume its own turn to expire the stun, so an
				// owner-activation stun could last forever; an infinite one always would.
				if (isStunStatus(status) && !std::holds_alternative<TimelineDurationSpec>(applyStatus->duration))
				{
					fail(
						p_owner,
						p_effect,
						"applies the stun status '" + applyStatus->status +
							"', which requires a finite timeline duration");
				}
			}
			else if (const auto *removeStatus = std::get_if<RemoveStatusEffectSpec>(&p_effect.payload))
			{
				(void)requireStatus(p_statuses, p_owner, p_effect, removeStatus->status);
			}
			else if (const auto *placeObject = std::get_if<PlaceObjectEffectSpec>(&p_effect.payload))
			{
				requireBattleObject(p_battleObjects, p_owner, p_effect, placeObject->object);
			}

			// cleanse and removeObjects select by tag, and a tag that matches nothing today is
			// legitimate: later content may introduce it. Their grammar was checked in phase 1.
		}

		void validateEffects(
			const Registry<StatusDefinition> &p_statuses,
			const Registry<BattleObjectDefinition> &p_battleObjects,
			const std::string &p_owner,
			const std::vector<EffectApplication> &p_effects)
		{
			for (const EffectApplication &effect : p_effects)
			{
				validateEffect(p_statuses, p_battleObjects, p_owner, effect);
			}
		}
	}

	void validateCombatDefinitionGraph(
		const Registry<StatusDefinition> &p_statuses,
		const Registry<AbilityDefinition> &p_abilities,
		const Registry<BattleObjectDefinition> &p_battleObjects)
	{
		for (const auto &[id, status] : p_statuses)
		{
			for (const StatusHookSpec &hook : status.hooks)
			{
				validateEffects(p_statuses, p_battleObjects, id, hook.effects);
			}
		}

		for (const auto &[id, battleObject] : p_battleObjects)
		{
			for (const BattleObjectTriggerSpec &trigger : battleObject.triggers)
			{
				validateEffects(p_statuses, p_battleObjects, id, trigger.effects);
			}
		}

		for (const auto &[id, ability] : p_abilities)
		{
			validateEffects(p_statuses, p_battleObjects, id, ability.effects);
		}
	}
}
