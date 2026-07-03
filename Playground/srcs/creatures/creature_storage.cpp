#include "creatures/creature_storage.hpp"

#include <algorithm>
#include <stdexcept>

namespace pg
{
	CreatureUnit &CreatureStorage::add(std::unique_ptr<CreatureUnit> p_creature)
	{
		if (p_creature == nullptr)
		{
			throw std::invalid_argument("cannot store a null creature");
		}
		CreatureUnit &result = *p_creature;
		_creatures.push_back(std::move(p_creature));
		return result;
	}

	std::unique_ptr<CreatureUnit> CreatureStorage::remove(CreatureUnit &p_creature)
	{
		const auto found = std::ranges::find_if(_creatures, [&](const auto &p_entry) {
			return p_entry.get() == &p_creature;
		});
		if (found == _creatures.end())
		{
			return nullptr;
		}
		std::unique_ptr<CreatureUnit> result = std::move(*found);
		_creatures.erase(found);
		return result;
	}

	std::size_t CreatureStorage::size() const noexcept
	{
		return _creatures.size();
	}
	bool CreatureStorage::empty() const noexcept
	{
		return _creatures.empty();
	}
	const std::vector<std::unique_ptr<CreatureUnit>> &CreatureStorage::creatures() const noexcept
	{
		return _creatures;
	}
}
