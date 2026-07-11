#pragma once

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/voxel/spk_voxel_chunk_renderer.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <utility>

namespace spk
{
	class Prefab;
	class VoxelMap;

	// A chunk of voxels living in the world as a regular game object: it owns its voxel
	// grid and comes with a VoxelChunkRenderer component baking that grid into a mesh.
	// Additional behaviors (physics, ...) can be attached later via addComponent.
	class VoxelChunk : public spk::Entity3D
	{
	public:
		inline static constexpr spk::Vector3Int Size{16, 16, 16};

		// A short-lived, write-only mutation surface used by editCells(). It deliberately
		// never exposes a mutable VoxelGrid or VoxelCell reference: an editor cannot be
		// copied, moved, or obtained outside the callback, so later writes cannot bypass
		// mesh invalidation. Several writes are committed as one synchronization request.
		class Editor
		{
			friend class spk::VoxelChunk;

		private:
			spk::VoxelChunk *_chunk = nullptr;
			std::uint8_t _changedBoundaries = 0;
			bool _changed = false;

			explicit Editor(spk::VoxelChunk &p_chunk) noexcept;

		public:
			Editor(const Editor &) = delete;
			Editor &operator=(const Editor &) = delete;
			Editor(Editor &&) noexcept = delete;
			Editor &operator=(Editor &&) noexcept = delete;

			[[nodiscard]] const spk::Vector3Int &size() const noexcept;
			[[nodiscard]] bool isWithinBounds(const spk::Vector3Int &p_position) const noexcept;
			[[nodiscard]] bool isWithinBounds(int p_x, int p_y, int p_z) const noexcept;
			[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_position) const;
			[[nodiscard]] const spk::VoxelCell &cell(int p_x, int p_y, int p_z) const;

			// Returns true only when the stored value changed.
			bool setCell(const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell);
			bool setCell(int p_x, int p_y, int p_z, const spk::VoxelCell &p_cell);
			void applyPrefab(
				const spk::Prefab &p_prefab,
				const spk::Vector3Int &p_destination,
				spk::VoxelOrientation p_orientation = spk::VoxelOrientation::PositiveZ);
		};

	private:
		friend class spk::VoxelMap;

		enum Boundary : std::uint8_t
		{
			NegativeXBoundary = 1U << 0U,
			PositiveXBoundary = 1U << 1U,
			NegativeYBoundary = 1U << 2U,
			PositiveYBoundary = 1U << 3U,
			NegativeZBoundary = 1U << 4U,
			PositiveZBoundary = 1U << 5U
		};

		spk::Vector3Int _coordinates;
		spk::VoxelGrid _grid{Size};
		spk::VoxelChunkRenderer *_renderer = nullptr;
		spk::VoxelMap *_owner = nullptr;
		// Owning-thread guard. Empty means unbound: while unbound, cells may be mutated from
		// any thread. A chunk is intentionally born unbound so single-threaded setup (warm-up,
		// generation) and standalone tooling/tests can edit it before the update thread exists.
		// VoxelMap binds this to the update thread once streaming begins (see bindMutationThread).
		std::optional<std::thread::id> _mutationThread;

		void _assertMutationThread() const;
		void _commit(Editor &p_editor) noexcept;

	public:
		explicit VoxelChunk(
			const spk::Vector3Int &p_coordinates,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup *p_worldLookup = nullptr,
			spk::Entity *p_parent = nullptr);
		// Owner-aware construction used by VoxelMap. Kept as a distinct five-argument
		// overload so the established four-argument parent contract remains source-compatible.
		VoxelChunk(
			const spk::Vector3Int &p_coordinates,
			const spk::VoxelRegistry &p_registry,
			const spk::IVoxelCellLookup *p_worldLookup,
			spk::VoxelMap *p_owner,
			spk::Entity *p_parent);

		[[nodiscard]] static spk::Vector3Int coordinatesFromWorldCell(const spk::Vector3Int &p_worldCell) noexcept;

		[[nodiscard]] const spk::Vector3Int &coordinates() const noexcept;
		[[nodiscard]] spk::Vector3Int worldOrigin() const noexcept;
		[[nodiscard]] spk::Vector3Int localFromWorld(const spk::Vector3Int &p_worldCell) const noexcept;

		[[nodiscard]] const spk::VoxelGrid &grid() const noexcept;

		// Binds cell mutation to p_thread: any later edit from another thread throws. This is
		// how the "owning update thread" is established. A chunk starts unbound (no check);
		// VoxelMap stamps its chunks with this once the update loop begins driving the map.
		void bindMutationThread(std::thread::id p_thread = std::this_thread::get_id()) noexcept;
		// Restores the unbound state: cell mutation becomes unrestricted again.
		void unbindMutationThread() noexcept;
		[[nodiscard]] bool hasBoundMutationThread() const noexcept;

		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_localPosition) const;

		// Voxel mutation is update-thread-owned. The callback must finish before mesh
		// synchronization starts; the engine's update pass joins chunk bake workers before
		// returning. Use this bulk path during generation instead of retaining grid aliases.
		template <typename TFunction>
		void editCells(TFunction &&p_function)
		{
			_assertMutationThread();
			Editor editor(*this);
			try
			{
				std::invoke(std::forward<TFunction>(p_function), editor);
			} catch (...)
			{
				_commit(editor);
				throw;
			}
			_commit(editor);
		}

		void setCell(const spk::Vector3Int &p_localPosition, const spk::VoxelCell &p_cell);

		[[nodiscard]] spk::VoxelChunkRenderer &renderer() noexcept;
		[[nodiscard]] const spk::VoxelChunkRenderer &renderer() const noexcept;
	};
}
