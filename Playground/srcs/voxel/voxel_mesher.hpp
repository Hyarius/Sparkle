#pragma once

#include "geometry/mesh3d.hpp"
#include "voxel/voxel_grid.hpp"
#include "voxel/voxel_registry.hpp"

#include <functional>
#include <span>

namespace pg
{
	class ICellLookup
	{
	public:
		virtual ~ICellLookup() = default;

		[[nodiscard]] virtual const VoxelCell *tryCell(const spk::Vector3Int &p_position) const = 0;
		[[nodiscard]] virtual const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const = 0;
	};

	class VoxelGridCellLookup final : public ICellLookup
	{
	private:
		const VoxelGrid &_grid;
		const VoxelRegistry &_registry;

	public:
		VoxelGridCellLookup(const VoxelGrid &p_grid, const VoxelRegistry &p_registry);

		[[nodiscard]] const VoxelCell *tryCell(const spk::Vector3Int &p_position) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const override;
	};

	class VoxelMesher
	{
	private:
		[[nodiscard]] static Mesh3D buildRenderMesh(
			const VoxelGrid &p_grid,
			const VoxelRegistry &p_registry,
			const ICellLookup *p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);

	public:
		using MaskAtlasLookup = std::function<AtlasCell(const VoxelCell &)>;

		[[nodiscard]] static Mesh3D buildRenderMesh(const VoxelGrid &p_grid, const VoxelRegistry &p_registry);
		[[nodiscard]] static Mesh3D buildRenderMesh(
			const VoxelGrid &p_grid,
			const VoxelRegistry &p_registry,
			const ICellLookup &p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);
		[[nodiscard]] static Mesh3D buildMaskMesh(
			std::span<const spk::Vector3Int> p_cells,
			const MaskAtlasLookup &p_maskOf,
			const ICellLookup &p_lookup);

		// Public for the orientation truth-table tests and future chunk-boundary lookup code.
		[[nodiscard]] static VoxelAxisPlane mapWorldPlaneToLocal(
			VoxelAxisPlane p_worldPlane,
			VoxelOrientation p_orientation,
			VoxelFlip p_flip);
	};
}
