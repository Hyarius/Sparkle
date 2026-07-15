#include "board/weighted_path.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <utility>

namespace pg
{
	namespace
	{
		// A settled or candidate route to one node. The cell sequence is carried whole rather
		// than reconstructed from predecessors: it IS the tie-break key, and a board is small
		// enough (at most 31 x 31 x 64 support cells) that copying it is not worth trading for a
		// predecessor chain that would have to reproduce the same full-sequence comparison anyway.
		struct Label
		{
			int cost = 0;
			std::vector<BoardCell> cells;

			[[nodiscard]] bool operator==(const Label &) const = default;
		};

		// The complete three-part key, in order: cost, then entered-cell count, then the
		// lexicographic cell sequence. Returns a negative value when p_left sorts first.
		[[nodiscard]] int compareLabels(const Label &p_left, const Label &p_right) noexcept
		{
			if (p_left.cost != p_right.cost)
			{
				return p_left.cost < p_right.cost ? -1 : 1;
			}
			if (p_left.cells.size() != p_right.cells.size())
			{
				return p_left.cells.size() < p_right.cells.size() ? -1 : 1;
			}

			const BoardCellLess less;
			for (std::size_t index = 0; index < p_left.cells.size(); ++index)
			{
				if (less(p_left.cells[index], p_right.cells[index]))
				{
					return -1;
				}
				if (less(p_right.cells[index], p_left.cells[index]))
				{
					return 1;
				}
			}
			return 0;
		}

		struct QueueEntry
		{
			Label label;
			std::size_t index = 0;
		};

		// std::priority_queue pops the greatest, so "greater" here means "settles later". The
		// node index is the last resort only: two entries reaching the same node with the same
		// complete label are the same route, and the graph index never leaks into the result.
		struct SettlesLater
		{
			[[nodiscard]] bool operator()(const QueueEntry &p_left, const QueueEntry &p_right) const noexcept
			{
				const int comparison = compareLabels(p_left.label, p_right.label);
				if (comparison != 0)
				{
					return comparison > 0;
				}
				return p_left.index > p_right.index;
			}
		};

		// Neighbours are visited in cell order, not in direction-slot order, so the search does
		// not inherit the builder's +X/-X/+Z/-Z convention as a hidden tie rule.
		[[nodiscard]] std::vector<std::size_t> sortedNeighbors(const TraversalGraph &p_graph, std::size_t p_index)
		{
			std::vector<std::size_t> result;
			result.reserve(4);
			for (const std::optional<std::size_t> &neighbor : p_graph.node(p_index).neighbors)
			{
				if (neighbor.has_value())
				{
					result.push_back(*neighbor);
				}
			}
			std::ranges::sort(result, [&p_graph](std::size_t p_left, std::size_t p_right) {
				return BoardCellLess{}(p_graph.node(p_left).position, p_graph.node(p_right).position);
			});
			result.erase(std::unique(result.begin(), result.end()), result.end());
			return result;
		}

		// One deterministic Dijkstra, shared by the path and the flood so they can never disagree
		// about which route is canonical. Returns the settled label of every reachable node.
		[[nodiscard]] std::vector<std::optional<Label>> settle(
			const TraversalGraph &p_graph,
			std::size_t p_source,
			int p_budget,
			const TraversalCostQuery &p_query)
		{
			std::vector<std::optional<Label>> best(p_graph.size());
			best[p_source] = Label{.cost = 0, .cells = {p_graph.node(p_source).position}};

			std::priority_queue<QueueEntry, std::vector<QueueEntry>, SettlesLater> open;
			open.push({.label = *best[p_source], .index = p_source});

			while (!open.empty())
			{
				QueueEntry current = open.top();
				open.pop();

				// Stale entries are detected by the complete label, not by cost alone: a route
				// that was replaced by a shorter-but-equally-cheap one must not be expanded.
				if (!best[current.index].has_value() || *best[current.index] != current.label)
				{
					continue;
				}

				for (const std::size_t neighbor : sortedNeighbors(p_graph, current.index))
				{
					const spk::Vector3Int &position = p_graph.node(neighbor).position;
					const std::optional<int> enterCost = p_query.enterCost(position);
					if (!enterCost.has_value())
					{
						continue;
					}
					if (*enterCost <= 0)
					{
						throw std::invalid_argument("a traversal enter cost must be strictly positive");
					}

					const std::optional<int> total = tryAdd(current.label.cost, *enterCost);
					if (!total.has_value() || *total > p_budget)
					{
						continue;
					}

					Label candidate{.cost = *total, .cells = current.label.cells};
					candidate.cells.push_back(position);
					if (best[neighbor].has_value() && compareLabels(*best[neighbor], candidate) <= 0)
					{
						continue;
					}
					best[neighbor] = candidate;
					open.push({.label = std::move(candidate), .index = neighbor});
				}
			}
			return best;
		}
	}

	TraversalCostQuery uniformCostQuery(int p_cost)
	{
		if (p_cost <= 0)
		{
			throw std::invalid_argument("a uniform traversal cost must be strictly positive");
		}
		return TraversalCostQuery{
			.enterCost = [p_cost](const spk::Vector3Int &) {
				return std::optional<int>(p_cost);
			}};
	}

	std::optional<WeightedPath> findWeightedPath(
		const TraversalGraph &p_graph,
		BoardCell p_source,
		BoardCell p_destination,
		int p_budget,
		const TraversalCostQuery &p_query)
	{
		const std::optional<std::size_t> source = p_graph.indexOf(p_source);
		const std::optional<std::size_t> destination = p_graph.indexOf(p_destination);
		if (!source.has_value() || !destination.has_value() || p_budget < 0)
		{
			return std::nullopt;
		}
		// The mover occupies its own cell, so the source is admitted at cost 0 whatever the
		// query says about entering it.
		if (*source == *destination)
		{
			return WeightedPath{.cells = {p_source}, .totalCost = 0};
		}

		const std::vector<std::optional<Label>> best = settle(p_graph, *source, p_budget, p_query);
		if (!best[*destination].has_value())
		{
			return std::nullopt;
		}
		return WeightedPath{.cells = best[*destination]->cells, .totalCost = best[*destination]->cost};
	}

	std::map<BoardCell, int, BoardCellLess> floodWeighted(
		const TraversalGraph &p_graph,
		BoardCell p_source,
		int p_budget,
		const TraversalCostQuery &p_query)
	{
		std::map<BoardCell, int, BoardCellLess> result;
		const std::optional<std::size_t> source = p_graph.indexOf(p_source);
		if (!source.has_value() || p_budget < 0)
		{
			return result;
		}

		const std::vector<std::optional<Label>> best = settle(p_graph, *source, p_budget, p_query);
		for (std::size_t index = 0; index < best.size(); ++index)
		{
			if (best[index].has_value())
			{
				result.emplace(p_graph.node(index).position, best[index]->cost);
			}
		}
		return result;
	}
}
