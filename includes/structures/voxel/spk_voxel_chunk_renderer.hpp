#pragma once

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_voxel_mesh_3d.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"

#include <cstdint>
#include <memory>

namespace spk
{
	// Render-side of a voxel chunk: bakes the chunk's grid into its opaque + transparent
	// meshes whenever a synchronization is requested (cell edited, neighbor loaded, ...).
	class VoxelChunkRenderer final : public spk::Component, public spk::SynchronizableTrait
	{
	public:
		using MeshHandle = std::shared_ptr<spk::VoxelRenderMeshes>;

	private:
		const spk::VoxelGrid *_grid = nullptr;
		const spk::VoxelRegistry *_registry = nullptr;
		const spk::IVoxelCellLookup *_worldLookup = nullptr;
		spk::Vector3Int _worldOrigin;
		// The baked meshes are held by shared_ptr and rebuilt as a brand-new pair (never
		// mutated in place) so a render command that captured the previous meshes keeps
		// drawing them safely on the render thread while _synchronize() swaps in the new
		// pair on the update thread.
		mutable std::shared_ptr<spk::VoxelRenderMeshes> _meshes = std::make_shared<spk::VoxelRenderMeshes>();
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

		[[nodiscard]] const spk::VoxelGrid &grid() const noexcept;

		[[nodiscard]] const spk::VoxelRenderMeshes &meshes() const noexcept;
		[[nodiscard]] std::shared_ptr<const spk::VoxelRenderMeshes> sharedMeshes() const noexcept;
		// Views into sharedMeshes() that keep the whole pair alive (for draw commands).
		[[nodiscard]] std::shared_ptr<const spk::VoxelMesh3D> sharedOpaqueMesh() const noexcept;
		[[nodiscard]] std::shared_ptr<const spk::VoxelMesh3D> sharedTransparentMesh() const noexcept;
		// Bumped on every re-synchronize so renderers can cache a chunk's draw and rebuild
		// it only when the baked meshes actually changed.
		[[nodiscard]] std::uint64_t meshRevision() const noexcept;
	};
}
