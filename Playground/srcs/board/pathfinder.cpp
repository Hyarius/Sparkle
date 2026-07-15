#include "board/pathfinder.hpp"

#include "board/weighted_path.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace
{
	// Every step costs one, so no route can enter more cells than the graph has nodes: that count
	// is an exact unbounded budget and, unlike a large sentinel, it cannot overflow a sum.
	[[nodiscard]] int unboundedBudget(const pg::TraversalGraph &p_graph) noexcept
	{
		return static_cast<int>(std::min<std::size_t>(p_graph.size(), std::numeric_limits<int>::max()));
	}

	// The float budget exploration authors becomes the integer budget the weighted search takes.
	// A fractional allowance buys no extra step, so it floors; a NaN budget buys nothing at all.
	[[nodiscard]] std::optional<int> integerBudget(const pg::TraversalGraph &p_graph, float p_maxCost) noexcept
	{
		if (std::isnan(p_maxCost) || p_maxCost < 0.0f)
		{
			return std::nullopt;
		}
		const float floored = std::floor(p_maxCost);
		const int ceiling = unboundedBudget(p_graph);
		if (floored >= static_cast<float>(ceiling))
		{
			return ceiling;
		}
		return static_cast<int>(floored);
	}
}

namespace pg
{
	std::optional<std::vector<spk::Vector3Int>> Pathfinder::findPath(
		const TraversalGraph &p_graph,
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to,
		std::optional<float> p_maxCost)
	{
		int budget = unboundedBudget(p_graph);
		if (p_maxCost.has_value())
		{
			const std::optional<int> bounded = integerBudget(p_graph, *p_maxCost);
			if (!bounded.has_value())
			{
				return std::nullopt;
			}
			budget = *bounded;
		}

		const std::optional<WeightedPath> path =
			findWeightedPath(p_graph, p_from, p_to, budget, uniformCostQuery(1));
		if (!path.has_value())
		{
			return std::nullopt;
		}
		return path->cells;
	}

	std::map<spk::Vector3Int, float, CellPositionLess> Pathfinder::floodReachable(
		const TraversalGraph &p_graph,
		const spk::Vector3Int &p_from,
		float p_maxCost)
	{
		std::map<spk::Vector3Int, float, CellPositionLess> result;
		const std::optional<int> budget = integerBudget(p_graph, p_maxCost);
		if (!budget.has_value())
		{
			return result;
		}

		for (const auto &[cell, cost] : floodWeighted(p_graph, p_from, *budget, uniformCostQuery(1)))
		{
			result.emplace(cell, static_cast<float>(cost));
		}
		return result;
	}
}
