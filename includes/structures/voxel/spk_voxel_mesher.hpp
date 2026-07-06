#pragma once

#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

namespace spk
{
	// Read access to voxel cells beyond a single grid, in world-cell coordinates.
	// Lets the mesher cull faces against neighboring chunks.
	class IVoxelCellLookup
	{
	public:
		virtual ~IVoxelCellLookup() = default;

		[[nodiscard]] virtual const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const = 0;

		[[nodiscard]] virtual const spk::VoxelCell *tryRenderableCell(const spk::Vector3Int &p_worldCell) const
		{
			return tryCell(p_worldCell);
		}
	};

	class VoxelMesher
	{
	private:
		[[nodiscard]] static spk::TextureMesh3D _buildRenderMesh(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup *p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);

	public:
		[[nodiscard]] static spk::TextureMesh3D buildRenderMesh(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry);
		[[nodiscard]] static spk::TextureMesh3D buildRenderMesh(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup &p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);

		// Public for the orientation truth-table tests and chunk-boundary lookup code.
		[[nodiscard]] static spk::VoxelAxisPlane mapWorldPlaneToLocal(
			spk::VoxelAxisPlane p_worldPlane,
			spk::VoxelOrientation p_orientation,
			spk::VoxelFlip p_flip);
	};
}
