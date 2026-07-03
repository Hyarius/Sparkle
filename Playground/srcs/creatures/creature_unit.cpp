#include "creatures/creature_unit.hpp"

#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"

#include <stdexcept>

namespace pg
{
	nlohmann::json FeatBoardProgress::toJson() const
	{
		return serialized;
	}

	void FeatBoardProgress::fromJson(const nlohmann::json &p_json)
	{
		serialized = p_json;
	}

	CreatureUnit::CreatureUnit(const CreatureSpecies *p_species) :
		species(p_species)
	{
		if (species != nullptr)
		{
			applyProgress(*this);
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
