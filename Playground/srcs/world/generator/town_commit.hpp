#pragma once

#include "world/generator/town_planner.hpp"

namespace pg
{
	// Strong transaction boundary: on failure p_plan remains bit-for-bit unchanged.
	void commitTownCandidate(WorldPlan &p_plan, const TownCandidate &p_candidate);
}
