#pragma once

#include "creatures/attributes.hpp"
#include "feats/feat_progress.hpp"

#include <string>
#include <vector>

namespace pg
{
	struct Ability;
	struct CreatureSpecies;
	struct Status;

	class CreatureUnit
	{
	public:
		const CreatureSpecies *species = nullptr;
		std::string currentFormId;
		Attributes attributes;
		int currentHealth = -1;
		std::vector<const Ability *> abilities;
		std::vector<const Status *> permanentPassives;
		FeatBoardProgress featBoardProgress;

		explicit CreatureUnit(const CreatureSpecies *p_species = nullptr);
		explicit CreatureUnit(const CreatureSpecies &p_species);
		[[nodiscard]] const std::string &displayName() const;
	};
}
