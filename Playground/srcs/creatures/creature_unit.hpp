#pragma once

#include "creatures/attributes.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace pg
{
	struct Ability;
	struct CreatureSpecies;
	struct Status;

	// Step-17 replaces this passthrough with UUID-keyed node progress.
	struct FeatBoardProgress
	{
		nlohmann::json serialized = nlohmann::json::object();

		[[nodiscard]] nlohmann::json toJson() const;
		void fromJson(const nlohmann::json &p_json);
	};

	class CreatureUnit
	{
	public:
		const CreatureSpecies *species = nullptr;
		std::string currentFormId;
		Attributes attributes;
		std::vector<const Ability *> abilities;
		std::vector<const Status *> permanentPassives;
		FeatBoardProgress featBoardProgress;

		explicit CreatureUnit(const CreatureSpecies *p_species = nullptr);
		explicit CreatureUnit(const CreatureSpecies &p_species);
		[[nodiscard]] const std::string &displayName() const;
	};
}
