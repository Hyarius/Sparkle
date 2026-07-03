#include "creatures/apply_progress.hpp"

#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"

#include <stdexcept>

namespace pg
{
	void applyProgress(CreatureUnit &p_unit)
	{
		if (p_unit.species == nullptr)
		{
			throw std::invalid_argument("applyProgress requires a creature species");
		}
		p_unit.attributes = p_unit.species->attributes;
		p_unit.abilities = p_unit.species->defaultAbilities;
		p_unit.permanentPassives.clear();
		p_unit.currentFormId = p_unit.species->defaultFormId;
	}
}
