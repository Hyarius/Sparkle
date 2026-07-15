#pragma once

#include "board/traversal_graph.hpp"

#include <map>
#include <optional>
#include <vector>

namespace pg
{
	// Exploration's flat-cost navigation: what the walking actor logic asks for, in the float
	// budget it has always used. It is now a thin skin over findWeightedPath with a uniform cost,
	// so an exploration route obeys the same canonical tie-break as a battle route and no longer
	// depends on where std::priority_queue happened to drop an equal-cost entry.
	//
	// Battle movement does not come through here: it pays per-cell terrain costs and treats
	// occupancy as a blocker, which is board/weighted_path.hpp.
	class Pathfinder
	{
	public:
		[[nodiscard]] static std::optional<std::vector<spk::Vector3Int>> findPath(
			const TraversalGraph &p_graph,
			const spk::Vector3Int &p_from,
			const spk::Vector3Int &p_to,
			std::optional<float> p_maxCost = std::nullopt);

		[[nodiscard]] static std::map<spk::Vector3Int, float, CellPositionLess> floodReachable(
			const TraversalGraph &p_graph,
			const spk::Vector3Int &p_from,
			float p_maxCost);
	};
}
