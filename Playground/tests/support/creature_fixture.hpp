#pragma once

#include "abilities/ability.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pg::test
{
	struct CreatureFixtureEntry
	{
		CreatureSpecies species;
		CreatureUnit unit;

		CreatureFixtureEntry(std::string p_name, Attributes p_attributes, std::vector<const Ability *> p_abilities) :
			species{
				.id = p_name,
				.displayName = p_name,
				.attributes = p_attributes,
				.defaultAbilities = std::move(p_abilities),
				.defaultFormId = "base",
				.forms = {{"base", CreatureForm{.displayName = std::move(p_name), .modelId = "placeholder-cube"}}}},
			unit(species)
		{
		}
	};

	inline CreatureUnit &creature(
		std::string p_name,
		Attributes p_attributes,
		std::vector<const Ability *> p_abilities = {})
	{
		static std::vector<std::unique_ptr<CreatureFixtureEntry>> entries;
		auto entry = std::make_unique<CreatureFixtureEntry>(
			std::move(p_name), p_attributes, std::move(p_abilities));
		CreatureUnit &result = entry->unit;
		entries.push_back(std::move(entry));
		return result;
	}
}
