#pragma once

#include "voxel/voxel_enums.hpp"

#include "structures/math/spk_vector3.hpp"

#include <optional>

namespace pg
{
	class VoxelWorld;

	struct WorldRay
	{
		spk::Vector3 origin{};
		spk::Vector3 direction{0, 0, 1};
	};

	struct VoxelHit
	{
		spk::Vector3Int cell{};
		// Axis plane for axis-aligned surfaces; Count for slopes and other oblique faces.
		VoxelAxisPlane enterFace = VoxelAxisPlane::Count;
		float distance = 0;
		spk::Vector3 normal{};
	};

	class WorldRaycaster
	{
	public:
		[[nodiscard]] static std::optional<VoxelHit> raycast(
			const VoxelWorld &p_world,
			const WorldRay &p_ray,
			float p_maxDistance);
	};
}
