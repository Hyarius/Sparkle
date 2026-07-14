#include "statuses/status_definition.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"

#include <algorithm>
#include <set>

namespace pg
{
	namespace
	{
		[[nodiscard]] StatModifierSpec parseStatModifier(JsonReader &p_reader)
		{
			StatModifierSpec result;
			result.stat = p_reader.requireEnum<CreatureStat>("stat", creatureStatNames());
			result.operation = p_reader.requireEnum<StatModifierOperation>("operation", statModifierOperationNames());

			// Stamina is milliseconds, so an additive stamina modifier is authored through its
			// own field. The unit is in the key, and a file cannot silently mix the two.
			const bool usesMilliseconds =
				result.stat == CreatureStat::Stamina && result.operation == StatModifierOperation::Add;
			if (usesMilliseconds)
			{
				p_reader.forbidUnknown({"id", "stat", "operation", "valueMilliseconds", "perStack"});
			}
			else
			{
				p_reader.forbidUnknown({"id", "stat", "operation", "value", "perStack"});
			}

			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "stat modifier id");

			const std::string valueKey = usesMilliseconds ? "valueMilliseconds" : "value";
			if (result.operation == StatModifierOperation::Add)
			{
				result.value =
					requireIntegerInRange(p_reader, valueKey, -MaximumAdditiveModifier, MaximumAdditiveModifier);
				if (result.value == 0)
				{
					throw JsonError(p_reader.file(), p_reader.pathFor(valueKey), "an additive modifier of zero does nothing");
				}
			}
			else
			{
				result.value =
					requireIntegerInRange(p_reader, valueKey, MinimumMultiplyPermille, MaximumMultiplyPermille);
			}

			result.perStack = p_reader.require<bool>("perStack");
			return result;
		}

		[[nodiscard]] std::vector<StatModifierSpec> parseStatModifiers(const JsonReader &p_reader)
		{
			std::vector<JsonReader> entries = p_reader.childArray("modifiers");

			std::vector<StatModifierSpec> result;
			result.reserve(entries.size());
			std::set<std::string> ids;
			for (JsonReader &entry : entries)
			{
				StatModifierSpec modifier = parseStatModifier(entry);
				if (!ids.insert(modifier.id).second)
				{
					throw JsonError(entry.file(), entry.path(), "duplicate stat modifier id '" + modifier.id + "'");
				}
				result.push_back(std::move(modifier));
			}
			return result;
		}

		[[nodiscard]] StatusHookSpec parseStatusHook(JsonReader &p_reader)
		{
			// "when" is the authored key; "hook" is the C++ field name and is not accepted.
			p_reader.forbidUnknown({"id", "when", "effects"});

			StatusHookSpec result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "status hook id");
			result.hook = p_reader.requireEnum<StatusHook>("when", statusHookNames());
			result.effects = parseEffects(p_reader, "effects");
			return result;
		}

		[[nodiscard]] std::vector<StatusHookSpec> parseStatusHooks(const JsonReader &p_reader)
		{
			std::vector<JsonReader> entries = p_reader.childArray("hooks");

			std::vector<StatusHookSpec> result;
			result.reserve(entries.size());
			std::set<std::string> hookIds;
			std::set<StatusHook> occupiedHooks;
			std::set<std::string> effectIds;
			for (JsonReader &entry : entries)
			{
				StatusHookSpec hook = parseStatusHook(entry);
				if (!hookIds.insert(hook.id).second)
				{
					throw JsonError(entry.file(), entry.path(), "duplicate status hook id '" + hook.id + "'");
				}
				// One entry per moment: two lists on the same hook would have no authored order
				// between them.
				if (!occupiedHooks.insert(hook.hook).second)
				{
					throw JsonError(
						entry.file(),
						entry.pathFor("when"),
						"this status already declares a hook for this moment; author one entry with an ordered "
						"effect list instead");
				}
				requireUniqueEffectIds(hook.effects, effectIds);
				result.push_back(std::move(hook));
			}
			return result;
		}

		void parseStacking(const JsonReader &p_reader, StatusDefinition &p_definition)
		{
			JsonReader stackingReader = p_reader.child("stacking");
			stackingReader.forbidUnknown({"maxStacks", "reapply", "durationRefresh"});

			p_definition.maxStacks = static_cast<std::uint32_t>(
				requireIntegerInRange(stackingReader, "maxStacks", 1, MaximumStatusStacks));
			p_definition.reapplyPolicy =
				stackingReader.requireEnum<StatusReapplyPolicy>("reapply", statusReapplyPolicyNames());
			p_definition.durationRefreshPolicy =
				stackingReader.requireEnum<DurationRefreshPolicy>("durationRefresh", durationRefreshPolicyNames());
		}
	}

	const std::map<std::string, StatusReapplyPolicy> &statusReapplyPolicyNames()
	{
		static const std::map<std::string, StatusReapplyPolicy> names{
			{"addStacks", StatusReapplyPolicy::AddStacks},
			{"replaceStacks", StatusReapplyPolicy::ReplaceStacks}};
		return names;
	}

	const std::map<std::string, DurationRefreshPolicy> &durationRefreshPolicyNames()
	{
		static const std::map<std::string, DurationRefreshPolicy> names{
			{"extend", DurationRefreshPolicy::Extend},
			{"keepLonger", DurationRefreshPolicy::KeepLonger},
			{"replace", DurationRefreshPolicy::Replace}};
		return names;
	}

	const std::map<std::string, StatModifierOperation> &statModifierOperationNames()
	{
		static const std::map<std::string, StatModifierOperation> names{
			{"add", StatModifierOperation::Add},
			{"multiplyPermille", StatModifierOperation::MultiplyPermille}};
		return names;
	}

	const std::map<std::string, StatusHook> &statusHookNames()
	{
		static const std::map<std::string, StatusHook> names{
			{"activationEnd", StatusHook::ActivationEnd},
			{"activationStart", StatusHook::ActivationStart},
			{"afterAbilityCast", StatusHook::AfterAbilityCast},
			{"afterDamageDealt", StatusHook::AfterDamageDealt},
			{"afterDamageTaken", StatusHook::AfterDamageTaken},
			{"afterHealingDone", StatusHook::AfterHealingDone},
			{"afterHealingReceived", StatusHook::AfterHealingReceived},
			{"afterVoluntaryMove", StatusHook::AfterVoluntaryMove},
			{"applied", StatusHook::Applied},
			{"removed", StatusHook::Removed}};
		return names;
	}

	bool isStunStatus(const StatusDefinition &p_status) noexcept
	{
		return std::ranges::find(p_status.tags, StunTag) != p_status.tags.end();
	}

	StatusDefinition parseStatusDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown(
			{"version", "displayNameKey", "descriptionKey", "icon", "tags", "stacking", "modifiers", "hooks"});
		requireVersion(p_reader, StatusSchemaVersion);

		StatusDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.descriptionKey = requireDescriptionKey(p_reader);
		result.icon = requireIcon(p_reader);
		result.tags = requireTags(p_reader, "tags", true);

		parseStacking(p_reader, result);
		result.modifiers = parseStatModifiers(p_reader);
		result.hooks = parseStatusHooks(p_reader);
		return result;
	}
}
