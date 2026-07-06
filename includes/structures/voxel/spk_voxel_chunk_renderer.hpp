#pragma once

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"

#include <cstdint>
#include <memory>

namespace spk
{
	// Render-side of a voxel chunk: bakes the chunk's grid into a mesh whenever a
	// synchronization is requested (cell edited, neighbor chunk loaded, ...).
	class VoxelChunkRenderer final : public spk::Component, public spk::SynchronizableTrait
	{
	public:
		using MeshHandle = std::shared_ptr<spk::TextureMesh3D>;

	private:
		const spk::VoxelGrid *_grid = nullptr;
		const spk::VoxelRegistry *_registry = nullptr;
		const spk::IVoxelCellLookup *_worldLookup = nullptr;
		spk::Vector3Int _worldOrigin;
		// The baked mesh is held by shared_ptr and rebuilt as a brand-new mesh (never mutated
		// in place) so a render command that captured the previous mesh keeps drawing it
		// safely on the render thread while _synchronize() swaps in the new one on the update
		// thread.
		mutable std::shared_ptr<spk::TextureMesh3D> _mesh = std::make_shared<spk::TextureMesh3D>();
		mutable std::uint64_t _meshRevision = 0;

	protected:
		void _synchronize() const override;

	public:
		VoxelChunkRenderer(
			const spk::VoxelGrid &p_grid,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup *p_worldLookup,
			const spk::Vector3Int &p_worldOrigin);

		[[nodiscard]] bool beginMeshSynchronization() const noexcept;
		[[nodiscard]] MeshHandle buildMesh() const;
		void publishMesh(MeshHandle p_mesh) const;
		void failMeshSynchronization() const noexcept;

		[[nodiscard]] const spk::TextureMesh3D &mesh() const noexcept;
		[[nodiscard]] std::shared_ptr<const spk::TextureMesh3D> sharedMesh() const noexcept;
		// Bumped on every re-synchronize so renderers can cache a chunk's draw and rebuild
		// it only when the baked mesh actually changed.
		[[nodiscard]] std::uint64_t meshRevision() const noexcept;
	};
}
