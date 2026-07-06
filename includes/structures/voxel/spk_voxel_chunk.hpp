#pragma once

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/voxel/spk_voxel_chunk_renderer.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"

namespace spk
{
	// A chunk of voxels living in the world as a regular game object: it owns its voxel
	// grid and comes with a VoxelChunkRenderer component baking that grid into a mesh.
	// Additional behaviors (physics, ...) can be attached later via addComponent.
	class VoxelChunk : public spk::Entity3D
	{
	public:
		inline static constexpr spk::Vector3Int Size{16, 16, 16};

	private:
		spk::Vector3Int _coordinates;
		spk::VoxelGrid _grid{Size};
		spk::VoxelChunkRenderer *_renderer = nullptr;

	public:
		explicit VoxelChunk(
			const spk::Vector3Int &p_coordinates,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup *p_worldLookup = nullptr,
			spk::Entity *p_parent = nullptr);

		[[nodiscard]] static spk::Vector3Int coordinatesFromWorldCell(const spk::Vector3Int &p_worldCell) noexcept;

		[[nodiscard]] const spk::Vector3Int &coordinates() const noexcept;
		[[nodiscard]] spk::Vector3Int worldOrigin() const noexcept;
		[[nodiscard]] spk::Vector3Int localFromWorld(const spk::Vector3Int &p_worldCell) const noexcept;

		[[nodiscard]] spk::VoxelGrid &grid() noexcept;
		[[nodiscard]] const spk::VoxelGrid &grid() const noexcept;

		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_localPosition) const;
		void setCell(const spk::Vector3Int &p_localPosition, const spk::VoxelCell &p_cell);

		[[nodiscard]] spk::VoxelChunkRenderer &renderer() noexcept;
		[[nodiscard]] const spk::VoxelChunkRenderer &renderer() const noexcept;
	};
}
