#include "board/board_raycaster.hpp"

#include "board/board_data.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	[[nodiscard]] bool solidAt(const pg::BoardData &p_board, const spk::Vector3Int &p_cell)
	{
		if (!p_board.isInside(p_cell))
		{
			return false;
		}
		const pg::VoxelCell &voxel = p_board.terrain().cells().cell(p_cell);
		const pg::VoxelDefinition *definition = p_board.terrain().cells().tryDefinition(voxel);
		return definition != nullptr && definition->data.traversal == pg::VoxelTraversal::Solid;
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
	std::optional<spk::Vector3Int> BoardRaycaster::resolveMouseCell(
		const BoardData &p_board,
		const WorldRay &p_worldRay,
		float p_maxDistance)
	{
		if (p_maxDistance < 0)
		{
			return std::nullopt;
		}
		if (p_worldRay.direction.isZero())
		{
			throw std::invalid_argument("Board ray direction cannot be zero");
		}
		const spk::Vector3 anchor(p_board.worldAnchor());
		const spk::Vector3 origin = p_worldRay.origin - anchor;
		const spk::Vector3 direction = p_worldRay.direction.normalized();
		spk::Vector3Int cell{
			static_cast<int>(std::floor(origin.x)),
			static_cast<int>(std::floor(origin.y)),
			static_cast<int>(std::floor(origin.z))};
		auto resolveColumn = [&p_board](const spk::Vector3Int &p_hit) -> std::optional<spk::Vector3Int> {
			if (p_hit.x < 0 || p_hit.z < 0 ||
				p_hit.x >= p_board.terrain().size().x || p_hit.z >= p_board.terrain().size().z)
			{
				return std::nullopt;
			}
			for (int y = std::min(p_hit.y, p_board.terrain().size().y - 1); y >= 0; --y)
			{
				const spk::Vector3Int candidate{p_hit.x, y, p_hit.z};
				if (p_board.isStandable(candidate))
				{
					return candidate;
				}
			}
			return std::nullopt;
		};
		if (solidAt(p_board, cell))
		{
			return resolveColumn(cell);
		}

		const int stepX = direction.x > 0 ? 1 : (direction.x < 0 ? -1 : 0);
		const int stepY = direction.y > 0 ? 1 : (direction.y < 0 ? -1 : 0);
		const int stepZ = direction.z > 0 ? 1 : (direction.z < 0 ? -1 : 0);
		float tMaxX = firstBoundary(origin.x, cell.x, stepX, direction.x);
		float tMaxY = firstBoundary(origin.y, cell.y, stepY, direction.y);
		float tMaxZ = firstBoundary(origin.z, cell.z, stepZ, direction.z);
		const float tDeltaX = stepX == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.x);
		const float tDeltaY = stepY == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.y);
		const float tDeltaZ = stepZ == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.z);
		float distance = 0.0f;
		while (distance <= p_maxDistance)
		{
			if (tMaxX <= tMaxY && tMaxX <= tMaxZ)
			{
				cell.x += stepX;
				distance = tMaxX;
				tMaxX += tDeltaX;
			}
			else if (tMaxY <= tMaxZ)
			{
				cell.y += stepY;
				distance = tMaxY;
				tMaxY += tDeltaY;
			}
			else
			{
				cell.z += stepZ;
				distance = tMaxZ;
				tMaxZ += tDeltaZ;
			}
			if (distance > p_maxDistance)
			{
				break;
			}
			if (solidAt(p_board, cell))
			{
				return resolveColumn(cell);
			}
		}
		return std::nullopt;
	}
}
