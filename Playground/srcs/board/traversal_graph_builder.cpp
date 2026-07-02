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

	[[nodiscard]] float edgeHeight(
		const pg::ICellSource &p_source,
		const spk::Vector3Int &p_position,
		pg::VoxelOrientation p_direction)
	{
		const pg::VoxelCell &cell = p_source.cell(p_position);
		const pg::VoxelDefinition *definition = p_source.tryDefinition(cell);
		if (definition == nullptr) return static_cast<float>(p_position.y);
		const pg::CardinalHeightSet heights = pg::resolveWorldHeights(
			definition->shape->heights(cell.flip), cell.orientation);
		return static_cast<float>(p_position.y) + heights.get(p_direction);
	}
}

namespace pg
{
	TraversalGraph TraversalGraphBuilder::build(
		const ICellSource &p_source,
		const TraversalBounds &p_bounds,
		float p_maxVerticalGap)
	{
		TraversalGraph graph;
		for (int y = p_bounds.minimum.y; y < p_bounds.maximum.y; ++y)
		{
			for (int z = p_bounds.minimum.z; z < p_bounds.maximum.z; ++z)
			{
				for (int x = p_bounds.minimum.x; x < p_bounds.maximum.x; ++x)
				{
					const spk::Vector3Int position{x, y, z};
					if (isStandable(p_source, position)) (void)graph.addNode(position);
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
				float bestGap = std::numeric_limits<float>::max();
				int bestVerticalDistance = std::numeric_limits<int>::max();
				std::optional<std::size_t> best;
				for (int y = p_bounds.minimum.y; y < p_bounds.maximum.y; ++y)
				{
					const spk::Vector3Int candidate{targetX, y, targetZ};
					const auto candidateIndex = graph.indexOf(candidate);
					if (!candidateIndex.has_value()) continue;
					const float gap = std::abs(
						edgeHeight(p_source, from, direction.orientation) -
						edgeHeight(p_source, candidate, direction.opposite));
					const int verticalDistance = std::abs(y - from.y);
					if (gap <= p_maxVerticalGap + 0.0001f &&
						(gap < bestGap - 0.0001f || (std::abs(gap - bestGap) <= 0.0001f && verticalDistance < bestVerticalDistance)))
					{
						bestGap = gap;
						bestVerticalDistance = verticalDistance;
						best = candidateIndex;
					}
				}
				if (best.has_value()) graph.connect(nodeIndex, directionIndex, *best);
			}
		}
		return graph;
	}
}
