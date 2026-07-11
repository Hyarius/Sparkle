#pragma once

#include "structures/graphics/geometry/spk_voxel_mesh_3d.hpp"
#include "structures/voxel/spk_voxel_cell_lookup.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

namespace spk
{
	class VoxelMesher
	{
	private:
		[[nodiscard]] static spk::VoxelRenderMeshes _buildRenderMesh(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup *p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);

	public:
		[[nodiscard]] static spk::VoxelRenderMeshes buildRenderMesh(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry);
		[[nodiscard]] static spk::VoxelRenderMeshes buildRenderMesh(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup &p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);

		// Compatibility wrapper; orientation code should call the free utility directly.
		[[nodiscard]] static spk::VoxelAxisPlane mapWorldPlaneToLocal(
			spk::VoxelAxisPlane p_worldPlane,
			spk::VoxelOrientation p_orientation,
			spk::VoxelFlip p_flip);
	};
}
