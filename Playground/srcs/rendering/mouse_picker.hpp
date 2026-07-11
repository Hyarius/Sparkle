#pragma once

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/math/spk_ray_3d.hpp"

#include <optional>

namespace pg
{
	class VoxelWorld;
	class WorldNavigation;

	class MousePicker
	{
	public:
		[[nodiscard]] static std::optional<spk::Vector3Int> pickStandable(
			const VoxelWorld &p_world,
			WorldNavigation &p_navigation,
			const spk::Ray3D &p_ray,
			float p_maxDistance = 1000.0f);
	};
}
