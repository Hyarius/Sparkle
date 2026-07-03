#pragma once

#include "structures/math/spk_vector2.hpp"

#include <string>
#include <vector>

namespace pg
{
	enum class PlacementStyle
	{
		Random,
		Fixed,
		ByLine
	};

	struct EncounterTeamMember
	{
		std::string speciesId;
		std::string aiId;
		std::vector<std::string> completedNodeUuids;
	};

	struct EncounterSpawn
	{
		std::string displayName;
		std::vector<EncounterTeamMember> enemyTeam;
		bool allowsTaming = true;
		spk::Vector2Int boardSize{11, 11};
		PlacementStyle placementStyle = PlacementStyle::Random;
	};
}
