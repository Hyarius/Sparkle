#include "abilities/ability_definition.hpp"

#include "core/definition_fields.hpp"

#include <set>

namespace pg
{
	namespace
	{
		[[nodiscard]] AbilityCost parseAbilityCost(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"actionPoints", "movementPoints"});

			AbilityCost result;
			result.actionPoints =
				static_cast<int>(requireIntegerInRange(p_reader, "actionPoints", 0, MaximumAbilityCost));
			result.movementPoints =
				static_cast<int>(requireIntegerInRange(p_reader, "movementPoints", 0, MaximumAbilityCost));
			return result;
		}
	}

	AbilityDefinition parseAbilityDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown(
			{"version",
			 "displayNameKey",
			 "descriptionKey",
			 "icon",
			 "cost",
			 "range",
			 "area",
			 "anchorFilter",
			 "affectedFilter",
			 "effects"});
		requireVersion(p_reader, AbilitySchemaVersion);

		AbilityDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.descriptionKey = requireDescriptionKey(p_reader);
		result.icon = requireIcon(p_reader);

		JsonReader costReader = p_reader.child("cost");
		result.cost = parseAbilityCost(costReader);

		JsonReader rangeReader = p_reader.child("range");
		result.range = parseRangeDefinition(rangeReader);

		JsonReader areaReader = p_reader.child("area");
		result.area = parseAreaDefinition(areaReader);

		JsonReader anchorReader = p_reader.child("anchorFilter");
		result.anchorFilter = parseTargetFilter(anchorReader);

		JsonReader affectedReader = p_reader.child("affectedFilter");
		result.affectedFilter = parseTargetFilter(affectedReader);

		// The two filters are independent by contract, but a self-anchored ability whose anchor
		// filter forbids the caster could never be cast at all.
		if (result.range.shape == RangeShape::Self && !result.anchorFilter.allowSelf)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("anchorFilter"),
				"a self range anchors on the caster, so anchorFilter.allowSelf must be true");
		}

		result.effects = parseEffects(p_reader, "effects");

		std::set<std::string> effectIds;
		requireUniqueEffectIds(result.effects, effectIds);
		return result;
	}
}
