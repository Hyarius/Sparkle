#pragma once

#include "creatures/creature_storage.hpp"
#include "structures/math/spk_vector3.hpp"

#include <array>
#include <memory>

namespace pg
{
	struct PlayerData
	{
		static constexpr std::size_t TeamCapacity = 6;

		spk::Vector3 position = spk::Vector3::Zero;
		int badgeCount = 0;
		std::array<std::unique_ptr<CreatureUnit>, TeamCapacity> team;
		CreatureStorage storage;

		CreatureUnit &addCreatureToTeamOrStorage(std::unique_ptr<CreatureUnit> p_creature);
		[[nodiscard]] std::size_t teamSize() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
	};
}
