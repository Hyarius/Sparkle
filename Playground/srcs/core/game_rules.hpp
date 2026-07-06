#pragma once

#include "core/json.hpp"

#include <array>
#include <map>
#include <string>

namespace pg
{
	struct GameRules
	{
		double maxVerticalTraversalGap = 0.0;
		std::map<std::string, std::array<int, 2>> overlayMasks;
	};

	[[nodiscard]] GameRules parseGameRules(JsonReader &p_reader);
}
