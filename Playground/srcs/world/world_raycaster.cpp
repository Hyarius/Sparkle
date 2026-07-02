#include "world/world_raycaster.hpp"

#include "world/voxel_world.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	[[nodiscard]] bool solidAt(const pg::VoxelWorld &p_world, const spk::Vector3Int &p_cell)
	{
		const pg::VoxelCell &cell = p_world.cell(p_cell);
		const pg::VoxelDefinition *definition = p_world.tryDefinition(cell);
		return definition != nullptr && definition->data.traversal == pg::VoxelTraversal::Solid;
	}

	[[nodiscard]] float firstBoundary(float p_origin, int p_cell, int p_step, float p_direction)
	{
		if (p_step == 0) return std::numeric_limits<float>::infinity();
		const float boundary = static_cast<float>(p_step > 0 ? p_cell + 1 : p_cell);
		return (boundary - p_origin) / p_direction;
	}
}

namespace pg
{
	std::optional<VoxelHit> WorldRaycaster::raycast(
		const VoxelWorld &p_world,
		const WorldRay &p_ray,
		float p_maxDistance)
	{
		if (p_maxDistance < 0) return std::nullopt;
		if (p_ray.direction.isZero()) throw std::invalid_argument("World ray direction cannot be zero");
		const spk::Vector3 direction = p_ray.direction.normalized();
		spk::Vector3Int cell{
			static_cast<int>(std::floor(p_ray.origin.x)),
			static_cast<int>(std::floor(p_ray.origin.y)),
			static_cast<int>(std::floor(p_ray.origin.z))};
		if (solidAt(p_world, cell)) return VoxelHit{cell, VoxelAxisPlane::Count, 0};

		const int stepX = direction.x > 0 ? 1 : (direction.x < 0 ? -1 : 0);
		const int stepY = direction.y > 0 ? 1 : (direction.y < 0 ? -1 : 0);
		const int stepZ = direction.z > 0 ? 1 : (direction.z < 0 ? -1 : 0);
		float tMaxX = firstBoundary(p_ray.origin.x, cell.x, stepX, direction.x);
		float tMaxY = firstBoundary(p_ray.origin.y, cell.y, stepY, direction.y);
		float tMaxZ = firstBoundary(p_ray.origin.z, cell.z, stepZ, direction.z);
		const float tDeltaX = stepX == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.x);
		const float tDeltaY = stepY == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.y);
		const float tDeltaZ = stepZ == 0 ? std::numeric_limits<float>::infinity() : std::abs(1.0f / direction.z);
		float distance = 0;
		VoxelAxisPlane face = VoxelAxisPlane::Count;
		while (distance <= p_maxDistance)
		{
			if (tMaxX <= tMaxY && tMaxX <= tMaxZ)
			{
				cell.x += stepX; distance = tMaxX; tMaxX += tDeltaX;
				face = stepX > 0 ? VoxelAxisPlane::NegativeX : VoxelAxisPlane::PositiveX;
			}
			else if (tMaxY <= tMaxZ)
			{
				cell.y += stepY; distance = tMaxY; tMaxY += tDeltaY;
				face = stepY > 0 ? VoxelAxisPlane::NegativeY : VoxelAxisPlane::PositiveY;
			}
			else
			{
				cell.z += stepZ; distance = tMaxZ; tMaxZ += tDeltaZ;
				face = stepZ > 0 ? VoxelAxisPlane::NegativeZ : VoxelAxisPlane::PositiveZ;
			}
			if (distance > p_maxDistance) break;
			if (solidAt(p_world, cell)) return VoxelHit{cell, face, distance};
		}
		return std::nullopt;
	}
}
