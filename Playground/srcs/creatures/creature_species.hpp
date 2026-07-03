#pragma once

#include "creatures/attributes.hpp"
#include "creatures/creature_form.hpp"

#include <map>
#include <string>
#include <vector>

namespace pg
{
	struct Ability;
	class JsonReader;
	template <typename TDefinition>
	class Registry;

	struct CreatureSpecies
	{
		std::string id;
		std::string displayName;
		Attributes attributes;
		std::vector<const Ability *> defaultAbilities;
		// Kept unresolved until the feat-board registry arrives in step 17.
		std::string featBoardId;
		std::string defaultFormId;
		std::map<std::string, CreatureForm> forms;

		[[nodiscard]] const CreatureForm &form(const std::string &p_id) const;
	};

	[[nodiscard]] CreatureSpecies parseCreatureSpecies(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities);
}
