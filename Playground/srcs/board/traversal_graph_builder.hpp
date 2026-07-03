#pragma once

#include "board/cell_source.hpp"
#include "board/traversal_graph.hpp"

#include "structures/math/spk_vector2.hpp"

#include <set>

namespace pg
{
	struct TraversalBounds
	{
		spk::Vector3Int minimum{};
		spk::Vector3Int maximum{}; // exclusive
	};

	struct CellColumnLess
	{
		[[nodiscard]] bool operator()(const spk::Vector2Int &p_left, const spk::Vector2Int &p_right) const noexcept
		{
			return std::tie(p_left.x, p_left.y) < std::tie(p_right.x, p_right.y);
		}
	};

	using ExcludedColumns = std::set<spk::Vector2Int, CellColumnLess>;

	class TraversalGraphBuilder
	{
	public:
		[[nodiscard]] static TraversalGraph build(
			const ICellSource &p_source,
			const TraversalBounds &p_bounds,
			float p_maxVerticalGap = 0.5f,
			const ExcludedColumns &p_excludedColumns = {});
	};
}
