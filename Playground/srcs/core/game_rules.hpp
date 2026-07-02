#pragma once

#include "core/json.hpp"

#include <array>
#include <map>
#include <string>

namespace pg
{
	struct GameRules
	{
		int teamMemberCount = 0;
		int abilityBarSlots = 0;
		double maxVerticalTraversalGap = 0.0;
		std::array<int, 2> defaultBoardSize{};
		int deploymentStripDepth = 0;
		double minimumTurnBarDuration = 0.0;
		double mitigationScaling = 0.0;
		double timeEffectResistanceScaling = 0.0;
		std::map<std::string, std::array<int, 2>> overlayMasks;
	};

	[[nodiscard]] GameRules parseGameRules(JsonReader &p_reader);
}
