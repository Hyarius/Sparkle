#pragma once

#include "board/traversal_graph.hpp"
#include "board/traversal_graph_builder.hpp"

#include <cstddef>
#include <optional>

namespace pg
{
	class VoxelWorld;

	class WorldNavigation
	{
	private:
		VoxelWorld &_world;
		TraversalBounds _bounds;
		TraversalGraph _graph;
		std::size_t _builtRevision = static_cast<std::size_t>(-1);
		float _maxVerticalGap = 0.5f;

	public:
		WorldNavigation(VoxelWorld &p_world, TraversalBounds p_bounds, float p_maxVerticalGap = 0.5f);
		void invalidate() noexcept;
		void refresh();
		[[nodiscard]] const TraversalGraph &graph();
		[[nodiscard]] std::optional<spk::Vector3Int> topStandableInColumn(int p_x, int p_z);
		[[nodiscard]] const TraversalBounds &bounds() const noexcept;
	};
}
