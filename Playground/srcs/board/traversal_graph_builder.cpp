#include "board/traversal_graph_builder.hpp"

#include "voxel/voxel_traversal.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace
{
	struct Direction
	{
		spk::Vector3Int offset;
		pg::VoxelOrientation orientation;
		pg::VoxelOrientation opposite;
	};

	constexpr std::array Directions = {
		Direction{{1, 0, 0}, pg::VoxelOrientation::PositiveX, pg::VoxelOrientation::NegativeX},
		Direction{{-1, 0, 0}, pg::VoxelOrientation::NegativeX, pg::VoxelOrientation::PositiveX},
		Direction{{0, 0, 1}, pg::VoxelOrientation::PositiveZ, pg::VoxelOrientation::NegativeZ},
		Direction{{0, 0, -1}, pg::VoxelOrientation::NegativeZ, pg::VoxelOrientation::PositiveZ}};

}

namespace pg
{
	TraversalGraph TraversalGraphBuilder::build(
		const ICellSource &p_source,
		const TraversalBounds &p_bounds,
		float p_maxVerticalGap,
		const ExcludedColumns &p_excludedColumns)
	{
		TraversalGraph graph;
		for (int y = p_bounds.minimum.y; y < p_bounds.maximum.y; ++y)
		{
			for (int z = p_bounds.minimum.z; z < p_bounds.maximum.z; ++z)
			{
				for (int x = p_bounds.minimum.x; x < p_bounds.maximum.x; ++x)
				{
					if (p_excludedColumns.contains({x, z}))
					{
						continue;
					}
					const spk::Vector3Int position{x, y, z};
					if (isStandable(p_source, position))
					{
						(void)graph.addNode(position);
					}
				}
			}
		}

		for (std::size_t nodeIndex = 0; nodeIndex < graph.size(); ++nodeIndex)
		{
			const spk::Vector3Int from = graph.node(nodeIndex).position;
			for (std::size_t directionIndex = 0; directionIndex < Directions.size(); ++directionIndex)
			{
				const Direction &direction = Directions[directionIndex];
				const int targetX = from.x + direction.offset.x;
				const int targetZ = from.z + direction.offset.z;
				if (p_excludedColumns.contains({targetX, targetZ}))
				{
					continue;
				}
				float bestGap = std::numeric_limits<float>::max();
				int bestVerticalDistance = std::numeric_limits<int>::max();
				std::optional<std::size_t> best;
				for (int y = p_bounds.minimum.y; y < p_bounds.maximum.y; ++y)
				{
					const spk::Vector3Int candidate{targetX, y, targetZ};
					const auto candidateIndex = graph.indexOf(candidate);
					if (!candidateIndex.has_value())
					{
						continue;
					}
					const float gap = std::abs(
						walkHeightAtEdge(p_source, from, direction.orientation) -
						walkHeightAtEdge(p_source, candidate, direction.opposite));
					const int verticalDistance = std::abs(y - from.y);
					if (gap <= p_maxVerticalGap + 0.0001f &&
						(gap < bestGap - 0.0001f || (std::abs(gap - bestGap) <= 0.0001f && verticalDistance < bestVerticalDistance)))
					{
						bestGap = gap;
						bestVerticalDistance = verticalDistance;
						best = candidateIndex;
					}
				}
				if (best.has_value())
				{
					graph.connect(nodeIndex, directionIndex, *best);
				}
			}
		}
		return graph;
	}
}
