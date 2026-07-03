#include "creatures/player_data.hpp"

#include <algorithm>
#include <stdexcept>

namespace pg
{
	CreatureUnit &PlayerData::addCreatureToTeamOrStorage(std::unique_ptr<CreatureUnit> p_creature)
	{
		if (p_creature == nullptr)
		{
			throw std::invalid_argument("cannot add a null creature");
		}
		for (std::unique_ptr<CreatureUnit> &slot : team)
		{
			if (slot == nullptr)
			{
				CreatureUnit &result = *p_creature;
				slot = std::move(p_creature);
				return result;
			}
		}
		return storage.add(std::move(p_creature));
	}

	std::size_t PlayerData::teamSize() const noexcept
	{
		return static_cast<std::size_t>(std::ranges::count_if(team, [](const auto &p_slot) {
			return p_slot != nullptr;
		}));
	}

	bool PlayerData::empty() const noexcept
	{
		return teamSize() == 0 && storage.empty();
	}
}
