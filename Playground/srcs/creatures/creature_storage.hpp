#pragma once

#include "creatures/creature_unit.hpp"

#include <memory>
#include <vector>

namespace pg
{
	class CreatureStorage
	{
	private:
		std::vector<std::unique_ptr<CreatureUnit>> _creatures;

	public:
		CreatureUnit &add(std::unique_ptr<CreatureUnit> p_creature);
		[[nodiscard]] std::unique_ptr<CreatureUnit> remove(CreatureUnit &p_creature);
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] const std::vector<std::unique_ptr<CreatureUnit>> &creatures() const noexcept;
	};
}
