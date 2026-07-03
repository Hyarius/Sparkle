#pragma once

#include <cstdint>
#include <memory>

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
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
		// Baked meshes are held by shared_ptr and rebuilt as brand-new meshes (never mutated
		// in place) so a render command that captured the previous mesh keeps drawing it
		// safely on the render thread while _synchronize() swaps in the new one on the update
		// thread.
		mutable std::shared_ptr<spk::TextureMesh3D> _renderMesh = std::make_shared<spk::TextureMesh3D>();
		mutable std::shared_ptr<spk::TextureMesh3D> _maskMesh = std::make_shared<spk::TextureMesh3D>();
		mutable std::uint64_t _meshRevision = 0;
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
		[[nodiscard]] const spk::TextureMesh3D &renderMesh() const noexcept;
		[[nodiscard]] const spk::TextureMesh3D &maskMesh() const noexcept;
		[[nodiscard]] std::shared_ptr<const spk::TextureMesh3D> sharedRenderMesh() const noexcept;
		[[nodiscard]] std::shared_ptr<const spk::TextureMesh3D> sharedMaskMesh() const noexcept;
		// Bumped on every re-synchronize so renderers can cache a chunk's draw and rebuild
		// it only when the baked mesh actually changed.
		[[nodiscard]] std::uint64_t meshRevision() const noexcept;

		void setCell(const spk::Vector3Int &p_localPosition, const VoxelCell &p_cell);
	};
}
