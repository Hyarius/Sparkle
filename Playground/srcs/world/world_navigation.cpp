#include "world/world_navigation.hpp"

#include "board/cell_source.hpp"
#include "world/voxel_world.hpp"

namespace pg
{
	WorldNavigation::WorldNavigation(VoxelWorld &p_world, TraversalBounds p_bounds, float p_maxVerticalGap) :
		_world(p_world), _bounds(p_bounds), _maxVerticalGap(p_maxVerticalGap)
	{
	}

	void WorldNavigation::invalidate() noexcept
	{
		_builtRevision = static_cast<std::size_t>(-1);
	}

	void WorldNavigation::resetBounds(TraversalBounds p_bounds) noexcept
	{
		_bounds = p_bounds;
		invalidate();
	}

	void WorldNavigation::refresh()
	{
		if (_builtRevision == _world.revision()) return;
		WorldCellSource source(_world);
		_graph = TraversalGraphBuilder::build(source, _bounds, _maxVerticalGap);
		_builtRevision = _world.revision();
	}

	const TraversalGraph &WorldNavigation::graph()
	{
		refresh();
		return _graph;
	}

	std::optional<spk::Vector3Int> WorldNavigation::topStandableInColumn(int p_x, int p_z)
	{
		refresh();
		for (int y = _bounds.maximum.y - 1; y >= _bounds.minimum.y; --y)
		{
			const spk::Vector3Int position{p_x, y, p_z};
			if (_graph.tryGetNode(position) != nullptr) return position;
		}
		return std::nullopt;
	}

	const TraversalBounds &WorldNavigation::bounds() const noexcept { return _bounds; }
}
