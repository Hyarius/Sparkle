#pragma once

#include "creatures/attributes.hpp"
#include "creatures/creature_form.hpp"
#include "taming/taming_profile.hpp"

#include <map>
#include <string>
#include <vector>

namespace pg
{
	struct Ability;
	class FeatBoard;
	class FeatRegistry;
	class JsonReader;
	template <typename TDefinition>
	class Registry;

	struct CreatureSpecies
	{
		std::string id;
		std::string displayName;
		Attributes attributes;
		std::vector<const Ability *> defaultAbilities;
		std::string featBoardId;
		const FeatBoard *featBoard = nullptr;
		std::string defaultFormId;
		std::map<std::string, CreatureForm> forms;
		TamingProfile tamingProfile;

		[[nodiscard]] const CreatureForm &form(const std::string &p_id) const;
	};

	[[nodiscard]] CreatureSpecies parseCreatureSpecies(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities);
	[[nodiscard]] CreatureSpecies parseCreatureSpecies(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const FeatRegistry &p_featBoards);
}
