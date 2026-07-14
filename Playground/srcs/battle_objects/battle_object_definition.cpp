#include "battle_objects/battle_object_definition.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"

#include <set>

namespace pg
{
	namespace
	{
		// An object triggers on the unit standing on its cell, so an empty cell can never fire
		// it, and a defeated unit is out of scope for v1.
		void requireTriggerFilter(const JsonReader &p_reader, const TargetFilter &p_filter)
		{
			if (p_filter.allowEmptyCell)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("targetFilter"),
					"an object triggers on a unit, so allowEmptyCell must be false");
			}
			if (!p_filter.allowSelf && !p_filter.allowAllies && !p_filter.allowEnemies)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("targetFilter"),
					"an object trigger must allow at least one of allowSelf, allowAllies or allowEnemies");
			}
		}

		[[nodiscard]] BattleObjectTriggerSpec parseTrigger(JsonReader &p_reader)
		{
			p_reader.forbidUnknown(
				{"id", "when", "targetFilter", "maxTriggers", "removeWhenExhausted", "effects"});

			BattleObjectTriggerSpec result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "battle object trigger id");
			result.trigger = p_reader.requireEnum<BattleObjectTrigger>("when", battleObjectTriggerNames());

			JsonReader filterReader = p_reader.child("targetFilter");
			result.targetFilter = parseTargetFilter(filterReader);
			requireTriggerFilter(p_reader, result.targetFilter);

			result.maxTriggers =
				static_cast<std::uint32_t>(requireIntegerInRange(p_reader, "maxTriggers", 0, MaximumObjectTriggers));
			result.removeWhenExhausted = p_reader.require<bool>("removeWhenExhausted");
			// An unlimited trigger is never exhausted, so a removal on exhaustion would be dead
			// authored intent rather than a rule.
			if (result.maxTriggers == 0 && result.removeWhenExhausted)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("removeWhenExhausted"),
					"an unlimited trigger (maxTriggers 0) is never exhausted, so it cannot remove the object on "
					"exhaustion");
			}

			result.effects = parseEffects(p_reader, "effects");
			return result;
		}
	}

	const std::map<std::string, BattleObjectTrigger> &battleObjectTriggerNames()
	{
		static const std::map<std::string, BattleObjectTrigger> names{
			{"unitActivationEndedOnCell", BattleObjectTrigger::UnitActivationEndedOnCell},
			{"unitActivationStartedOnCell", BattleObjectTrigger::UnitActivationStartedOnCell},
			{"unitEndedMoveOnCell", BattleObjectTrigger::UnitEndedMoveOnCell},
			{"unitEnteredCell", BattleObjectTrigger::UnitEnteredCell},
			{"unitLeftCell", BattleObjectTrigger::UnitLeftCell}};
		return names;
	}

	BattleObjectDefinition parseBattleObjectDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown(
			{"version",
			 "displayNameKey",
			 "descriptionKey",
			 "icon",
			 "tags",
			 "blocksMovement",
			 "blocksLineOfSight",
			 "triggers"});
		requireVersion(p_reader, BattleObjectSchemaVersion);

		BattleObjectDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.descriptionKey = requireDescriptionKey(p_reader);
		result.icon = requireIcon(p_reader);
		result.tags = requireTags(p_reader, "tags", true);
		result.blocksMovement = p_reader.require<bool>("blocksMovement");
		result.blocksLineOfSight = p_reader.require<bool>("blocksLineOfSight");

		std::vector<JsonReader> entries = p_reader.childArray("triggers");
		if (entries.empty())
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("triggers"),
				"an object that never triggers does nothing; author at least one trigger");
		}

		std::set<std::string> triggerIds;
		std::set<std::string> effectIds;
		result.triggers.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			BattleObjectTriggerSpec trigger = parseTrigger(entry);
			if (!triggerIds.insert(trigger.id).second)
			{
				throw JsonError(entry.file(), entry.path(), "duplicate trigger id '" + trigger.id + "'");
			}
			requireUniqueEffectIds(trigger.effects, effectIds);
			result.triggers.push_back(std::move(trigger));
		}
		return result;
	}
}
