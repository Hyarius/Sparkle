#pragma once

#include "board/cell_source.hpp"
#include "board/traversal_graph.hpp"

namespace pg
{
	struct TraversalBounds
	{
		spk::Vector3Int minimum{};
		spk::Vector3Int maximum{}; // exclusive
	};

	class TraversalGraphBuilder
	{
	public:
		[[nodiscard]] static TraversalGraph build(
			const ICellSource &p_source,
			const TraversalBounds &p_bounds,
			float p_maxVerticalGap = 0.5f);
	};
}
