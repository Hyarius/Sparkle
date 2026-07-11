#include "rendering/mouse_picker.hpp"

#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include "structures/voxel/spk_voxel_ray_cast.hpp"

namespace pg
{
	std::optional<spk::Vector3Int> MousePicker::pickStandable(
		const VoxelWorld &p_world,
		WorldNavigation &p_navigation,
		const spk::Ray3D &p_ray,
		float p_maxDistance)
	{
		const auto hit = spk::VoxelRayCast::cast(
			p_world.map(),
			p_ray,
			p_maxDistance,
			[&p_world](const spk::Vector3Int &, const spk::VoxelCell &p_cell) {
				const VoxelDefinition *definition = p_world.tryDefinition(p_cell);
				return definition != nullptr && definition->data.traversal == VoxelTraversal::Solid;
			});
		if (!hit.has_value())
		{
			return std::nullopt;
		}
		const TraversalBounds &bounds = p_navigation.bounds();
		for (int y = std::min(hit->cell.y, bounds.maximum.y - 1); y >= bounds.minimum.y; --y)
		{
			const spk::Vector3Int candidate{hit->cell.x, y, hit->cell.z};
			if (p_navigation.graph().tryGetNode(candidate) != nullptr)
			{
				return candidate;
			}
		}
		return std::nullopt;
	}
}
