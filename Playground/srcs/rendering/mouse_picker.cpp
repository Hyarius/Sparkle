#include "rendering/mouse_picker.hpp"

#include "components/camera3d.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include "structures/math/spk_vector4.hpp"

#include <stdexcept>

namespace pg
{
	WorldRay MousePicker::screenToRay(
		const Camera3D &p_camera,
		const spk::Vector2 &p_viewportSize,
		const spk::Vector2 &p_mousePixels)
	{
		if (p_viewportSize.x <= 0 || p_viewportSize.y <= 0)
		{
			throw std::invalid_argument("Mouse picker viewport must be positive");
		}
		const float ndcX = 2.0f * p_mousePixels.x / p_viewportSize.x - 1.0f;
		const float ndcY = 1.0f - 2.0f * p_mousePixels.y / p_viewportSize.y;
		const spk::Matrix4x4 inverse = p_camera.viewProjectionMatrix().inverse();
		spk::Vector4 nearPoint = inverse * spk::Vector4{ndcX, ndcY, -1.0f, 1.0f};
		spk::Vector4 farPoint = inverse * spk::Vector4{ndcX, ndcY, 1.0f, 1.0f};
		nearPoint /= nearPoint.w;
		farPoint /= farPoint.w;
		return {
			.origin = p_camera.position(),
			.direction = (farPoint.xyz() - nearPoint.xyz()).normalized()};
	}

	std::optional<spk::Vector3Int> MousePicker::pickStandable(
		const VoxelWorld &p_world,
		WorldNavigation &p_navigation,
		const WorldRay &p_ray,
		float p_maxDistance)
	{
		const auto hit = WorldRaycaster::raycast(p_world, p_ray, p_maxDistance);
		if (!hit.has_value()) return std::nullopt;
		const TraversalBounds &bounds = p_navigation.bounds();
		for (int y = std::min(hit->cell.y, bounds.maximum.y - 1); y >= bounds.minimum.y; --y)
		{
			const spk::Vector3Int candidate{hit->cell.x, y, hit->cell.z};
			if (p_navigation.graph().tryGetNode(candidate) != nullptr) return candidate;
		}
		return std::nullopt;
	}
}
