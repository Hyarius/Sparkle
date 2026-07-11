#pragma once

#include "structures/math/spk_ray_3d.hpp"
#include "structures/voxel/spk_voxel_cell_lookup.hpp"

#include <functional>
#include <optional>

namespace spk
{
	// Lightweight grid traversal. A VoxelRayCast intersects unit voxel boundaries
	// only; it deliberately does not inspect a voxel shape's polygons.
	class VoxelRayCast
	{
	public:
		struct Hit
		{
			spk::Vector3Int cell{};
			float distance = 0.0f;
			// Points out of the entered cell. Edge/corner entries can contain two or
			// three non-zero components; a ray starting inside a cell reports zero.
			spk::Vector3Int entryNormal{};
		};

		using CellPredicate = std::function<bool(const spk::Vector3Int &, const spk::VoxelCell &)>;

		[[nodiscard]] static std::optional<Hit> cast(
			const spk::IVoxelCellLookup &p_cells,
			const spk::Ray3D &p_ray,
			float p_maxDistance,
			CellPredicate p_selectable = nullptr);
	};
}
