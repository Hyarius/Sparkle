#pragma once

#include "board/traversal_graph_builder.hpp"

namespace pg
{
	class BoardTerrainLayer;

	class BoardNavigationLayer
	{
	private:
		TraversalGraph _graph;

	public:
		BoardNavigationLayer() = default;
		explicit BoardNavigationLayer(
			const BoardTerrainLayer &p_terrain,
			float p_maxVerticalGap = 0.5f,
			const ExcludedColumns &p_excludedColumns = {});

		void rebuild(
			const BoardTerrainLayer &p_terrain,
			float p_maxVerticalGap = 0.5f,
			const ExcludedColumns &p_excludedColumns = {});
		[[nodiscard]] bool isStandable(const spk::Vector3Int &p_local) const noexcept;
		[[nodiscard]] const TraversalGraph &graph() const noexcept;
	};
}
