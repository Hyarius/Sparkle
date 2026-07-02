#pragma once

#include "world/world_raycaster.hpp"

#include "structures/math/spk_vector2.hpp"

#include <optional>

namespace pg
{
	class Camera3D;
	class VoxelWorld;
	class WorldNavigation;

	class MousePicker
	{
	public:
		[[nodiscard]] static WorldRay screenToRay(
			const Camera3D &p_camera,
			const spk::Vector2 &p_viewportSize,
			const spk::Vector2 &p_mousePixels);
		[[nodiscard]] static std::optional<spk::Vector3Int> pickStandable(
			const VoxelWorld &p_world,
			WorldNavigation &p_navigation,
			const WorldRay &p_ray,
			float p_maxDistance = 1000.0f);
	};
}
