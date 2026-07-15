#pragma once

#include "board/board_cell.hpp"
#include "board/traversal_graph.hpp"

#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace pg
{
	// A movement route through the traversal graph. The source is part of the sequence but is
	// never paid for: totalCost sums the enter cost of every cell after it.
	struct WeightedPath
	{
		std::vector<BoardCell> cells; // includes source and destination
		int totalCost = 0;            // excludes the source; sums the cells entered

		[[nodiscard]] bool operator==(const WeightedPath &) const = default;
	};

	// What entering a cell costs the mover. nullopt means blocked - for both traversal and
	// destination; there is no ally pass-through in v1. Any returned cost must be strictly
	// positive, so a cycle can never be free and the search always terminates.
	//
	// The query is pure and is called many times per search. It borrows whatever it reads (a
	// BoardData, an occupancy map), so its subject has to outlive the call.
	struct TraversalCostQuery
	{
		std::function<std::optional<int>(const spk::Vector3Int &)> enterCost;
	};

	// Every cell costs the same. This is what the legacy exploration pathfinder became: the
	// unweighted search is the weighted one with a flat cost, so both obey the same tie-breaks.
	[[nodiscard]] TraversalCostQuery uniformCostQuery(int p_cost = 1);

	// The canonical minimum-cost route, and the canonical reachable set.
	//
	// Ties are not left to the priority queue: a label's complete comparison key is
	//   (total entered-cell cost, entered-cell count, lexicographic complete cell sequence),
	// so two graphs holding the same cells and edges in a different insertion order return the
	// exact same path. A relaxation replaces the incumbent label whenever the complete key is
	// smaller, not merely when the cost is.
	//
	// from == to yields {from} at cost 0. A missing endpoint, a negative budget, a blocked
	// destination, or a cheapest route over budget yields no path. The flood includes the source
	// at 0; callers may drop it from a movement highlight.
	[[nodiscard]] std::optional<WeightedPath> findWeightedPath(
		const TraversalGraph &p_graph,
		BoardCell p_source,
		BoardCell p_destination,
		int p_budget,
		const TraversalCostQuery &p_query);

	[[nodiscard]] std::map<BoardCell, int, BoardCellLess> floodWeighted(
		const TraversalGraph &p_graph,
		BoardCell p_source,
		int p_budget,
		const TraversalCostQuery &p_query);
}
