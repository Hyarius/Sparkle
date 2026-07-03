#include "board/line_of_sight.hpp"

#include "voxel/voxel_traversal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	[[nodiscard]] float standingHeight(const pg::ICellSource &p_source, const spk::Vector3Int &p_cell)
	{
		const pg::VoxelCell &voxel = p_source.cell(p_cell);
		const pg::VoxelDefinition *definition = p_source.tryDefinition(voxel);
		if (definition == nullptr)
		{
			return static_cast<float>(p_cell.y);
		}
		return static_cast<float>(p_cell.y) +
			   pg::resolveWorldHeights(definition->shape->heights(voxel.flip), voxel.orientation).stationary;
	}

	[[nodiscard]] bool blocksLine(
		const pg::ICellSource &p_source,
		const spk::Vector3Int &p_cell,
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to)
	{
		if ((p_cell.x == p_from.x && p_cell.z == p_from.z) ||
			(p_cell.x == p_to.x && p_cell.z == p_to.z))
		{
			return false;
		}
		const pg::VoxelCell &voxel = p_source.cell(p_cell);
		const pg::VoxelDefinition *definition = p_source.tryDefinition(voxel);
		return definition != nullptr && definition->data.traversal == pg::VoxelTraversal::Solid &&
			   !definition->data.hasTag("losTransparent");
	}

	[[nodiscard]] float firstBoundary(float p_origin, int p_cell, int p_step, float p_direction)
	{
		if (p_step == 0)
		{
			return std::numeric_limits<float>::infinity();
		}
		const float boundary = static_cast<float>(p_step > 0 ? p_cell + 1 : p_cell);
		return (boundary - p_origin) / p_direction;
	}
}

namespace pg
{
	bool LineOfSight::hasLine(
		const ICellSource &p_source,
		const spk::Vector3Int &p_fromCell,
		const spk::Vector3Int &p_toCell)
	{
		if (p_fromCell == p_toCell ||
			std::abs(p_fromCell.x - p_toCell.x) + std::abs(p_fromCell.z - p_toCell.z) <= 1)
		{
			return true;
		}
		const spk::Vector3 start{
			static_cast<float>(p_fromCell.x) + 0.5f,
			standingHeight(p_source, p_fromCell) + 0.5f,
			static_cast<float>(p_fromCell.z) + 0.5f};
		const spk::Vector3 end{
			static_cast<float>(p_toCell.x) + 0.5f,
			standingHeight(p_source, p_toCell) + 0.5f,
			static_cast<float>(p_toCell.z) + 0.5f};
		const spk::Vector3 direction = end - start;
		spk::Vector3Int cell{
			static_cast<int>(std::floor(start.x)),
			static_cast<int>(std::floor(start.y)),
			static_cast<int>(std::floor(start.z))};
		const int stepX = direction.x > 0 ? 1 : (direction.x < 0 ? -1 : 0);
		const int stepY = direction.y > 0 ? 1 : (direction.y < 0 ? -1 : 0);
		const int stepZ = direction.z > 0 ? 1 : (direction.z < 0 ? -1 : 0);
		float tMaxX = firstBoundary(start.x, cell.x, stepX, direction.x);
		float tMaxY = firstBoundary(start.y, cell.y, stepY, direction.y);
		float tMaxZ = firstBoundary(start.z, cell.z, stepZ, direction.z);
		const float tDeltaX = stepX == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.x);
		const float tDeltaY = stepY == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.y);
		const float tDeltaZ = stepZ == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.z);
		while (true)
		{
			const float next = std::min({tMaxX, tMaxY, tMaxZ});
			if (next > 1.0f)
			{
				break;
			}
			if (std::abs(tMaxX - next) <= 0.00001f)
			{
				cell.x += stepX;
				tMaxX += tDeltaX;
			}
			if (std::abs(tMaxY - next) <= 0.00001f)
			{
				cell.y += stepY;
				tMaxY += tDeltaY;
			}
			if (std::abs(tMaxZ - next) <= 0.00001f)
			{
				cell.z += stepZ;
				tMaxZ += tDeltaZ;
			}
			if (blocksLine(p_source, cell, p_fromCell, p_toCell))
			{
				return false;
			}
		}
		return true;
	}
}
