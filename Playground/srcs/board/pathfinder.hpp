#pragma once

#include "board/traversal_graph.hpp"

#include <map>
#include <optional>
#include <vector>

namespace pg
{
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
