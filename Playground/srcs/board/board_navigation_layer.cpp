#include "board/board_navigation_layer.hpp"

#include "board/board_terrain_layer.hpp"

namespace pg
{
	BoardNavigationLayer::BoardNavigationLayer(
		const BoardTerrainLayer &p_terrain,
		float p_maxVerticalGap,
		const ExcludedColumns &p_excludedColumns)
	{
		rebuild(p_terrain, p_maxVerticalGap, p_excludedColumns);
	}

	void BoardNavigationLayer::rebuild(
		const BoardTerrainLayer &p_terrain,
		float p_maxVerticalGap,
		const ExcludedColumns &p_excludedColumns)
	{
		_graph = TraversalGraphBuilder::build(
			p_terrain.cells(),
			{{0, 0, 0}, p_terrain.size()},
			p_maxVerticalGap,
			p_excludedColumns);
	}

	bool BoardNavigationLayer::isStandable(const spk::Vector3Int &p_local) const noexcept
	{
		return _graph.tryGetNode(p_local) != nullptr;
	}

	const TraversalGraph &BoardNavigationLayer::graph() const noexcept
	{
		return _graph;
	}
}
