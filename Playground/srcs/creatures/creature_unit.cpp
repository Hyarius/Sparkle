#include "creatures/creature_unit.hpp"

#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"
#include "feats/feat_board.hpp"

#include <stdexcept>

namespace pg
{
	CreatureUnit::CreatureUnit(const CreatureSpecies *p_species) :
		species(p_species)
	{
		if (species != nullptr)
		{
			if (species->featBoard != nullptr)
			{
				featBoardProgress.getOrCreateProgress(species->featBoard->node(species->featBoard->rootNodeUuid)).completionCount = 1;
			}
			applyProgress(*this);
			currentHealth = attributes.health;
		}
	}

	CreatureUnit::CreatureUnit(const CreatureSpecies &p_species) :
		CreatureUnit(&p_species)
	{
	}

	const std::string &CreatureUnit::displayName() const
	{
		if (species == nullptr)
		{
			throw std::logic_error("creature unit has no species");
		}
		return species->form(currentFormId).displayName;
	}
}
