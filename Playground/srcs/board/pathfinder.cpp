#include "board/pathfinder.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace
{
	struct QueueEntry
	{
		float priority = 0;
		std::size_t index = 0;
		[[nodiscard]] bool operator>(const QueueEntry &p_other) const noexcept { return priority > p_other.priority; }
	};

	[[nodiscard]] float heuristic(const spk::Vector3Int &p_from, const spk::Vector3Int &p_to)
	{
		return static_cast<float>(std::abs(p_from.x - p_to.x) + std::abs(p_from.z - p_to.z));
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
		const auto start = p_graph.indexOf(p_from);
		const auto goal = p_graph.indexOf(p_to);
		if (!start.has_value() || !goal.has_value()) return std::nullopt;

		std::vector<float> costs(p_graph.size(), std::numeric_limits<float>::infinity());
		std::vector<std::optional<std::size_t>> previous(p_graph.size());
		std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> open;
		costs[*start] = 0;
		open.push({heuristic(p_from, p_to), *start});
		while (!open.empty())
		{
			const std::size_t current = open.top().index;
			open.pop();
			if (current == *goal) break;
			for (const auto neighbor : p_graph.node(current).neighbors)
			{
				if (!neighbor.has_value()) continue;
				const float nextCost = costs[current] + 1.0f;
				if (p_maxCost.has_value() && nextCost > *p_maxCost) continue;
				if (nextCost < costs[*neighbor])
				{
					costs[*neighbor] = nextCost;
					previous[*neighbor] = current;
					open.push({nextCost + heuristic(p_graph.node(*neighbor).position, p_to), *neighbor});
				}
			}
		}
		if (!std::isfinite(costs[*goal])) return std::nullopt;
		std::vector<spk::Vector3Int> path;
		for (std::optional<std::size_t> current = goal; current.has_value(); current = previous[*current])
		{
			path.push_back(p_graph.node(*current).position);
			if (*current == *start) break;
		}
		std::ranges::reverse(path);
		return path;
	}

	std::map<spk::Vector3Int, float, CellPositionLess> Pathfinder::floodReachable(
		const TraversalGraph &p_graph,
		const spk::Vector3Int &p_from,
		float p_maxCost)
	{
		std::map<spk::Vector3Int, float, CellPositionLess> result;
		const auto start = p_graph.indexOf(p_from);
		if (!start.has_value() || p_maxCost < 0) return result;
		std::vector<float> costs(p_graph.size(), std::numeric_limits<float>::infinity());
		std::queue<std::size_t> open;
		costs[*start] = 0;
		open.push(*start);
		while (!open.empty())
		{
			const std::size_t current = open.front(); open.pop();
			result[p_graph.node(current).position] = costs[current];
			for (const auto neighbor : p_graph.node(current).neighbors)
			{
				if (!neighbor.has_value()) continue;
				const float next = costs[current] + 1.0f;
				if (next <= p_maxCost && next < costs[*neighbor])
				{
					costs[*neighbor] = next;
					open.push(*neighbor);
				}
			}
		}
		return result;
	}
}
