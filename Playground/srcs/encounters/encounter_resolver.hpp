#pragma once

#include "encounters/encounter_types.hpp"
#include "structures/math/spk_vector3.hpp"

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace pg
{
	struct BiomeEncounterRule;

	struct ResolvedEncounter
	{
		std::string displayName;
		std::vector<EncounterTeamMember> team;
	};

	class EncounterResolver
	{
	private:
		std::mt19937 _random;
		std::optional<spk::Vector3Int> _lastCheckedCell;

	public:
		static constexpr unsigned int StreamId = 0x454E4354u;

		explicit EncounterResolver(int p_worldSeed);

		[[nodiscard]] std::optional<ResolvedEncounter> tryRoll(
			const BiomeEncounterRule &p_rule,
			const spk::Vector3Int &p_cell,
			int p_badgeCount);
		void observeCell(const spk::Vector3Int &p_cell) noexcept;
	};
}
