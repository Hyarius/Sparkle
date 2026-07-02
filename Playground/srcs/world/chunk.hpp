#pragma once

#include "geometry/mesh3d.hpp"
#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "voxel/voxel_grid.hpp"
#include "world/chunk_coordinates.hpp"

namespace pg
{
	class ICellLookup;
	class VoxelRegistry;

	class Chunk final : public spk::Component, public spk::SynchronizableTrait
	{
	public:
		inline static constexpr spk::Vector3Int Size{16, 16, 16};

	private:
		ChunkCoordinates _coordinates;
		VoxelGrid _grid{Size};
		mutable Mesh3D _renderMesh;
		mutable Mesh3D _maskMesh;
		const VoxelRegistry *_registry = nullptr;
		const ICellLookup *_worldLookup = nullptr;

	protected:
		void _synchronize() const override;

	public:
		Chunk(
			ChunkCoordinates p_coordinates,
			const VoxelRegistry &p_registry,
			const ICellLookup &p_worldLookup);

		[[nodiscard]] const ChunkCoordinates &coordinates() const noexcept;
		[[nodiscard]] VoxelGrid &grid() noexcept;
		[[nodiscard]] const VoxelGrid &grid() const noexcept;
		[[nodiscard]] const Mesh3D &renderMesh() const noexcept;
		[[nodiscard]] const Mesh3D &maskMesh() const noexcept;

		void setCell(const spk::Vector3Int &p_localPosition, const VoxelCell &p_cell);
	};
}
