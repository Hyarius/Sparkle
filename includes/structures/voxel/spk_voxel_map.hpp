#pragma once

#include "structures/voxel/spk_prefab.hpp"
#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_cell_lookup.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace spk
{
	class GameEngine;

	// Streams voxel chunks on demand: the map owns the chunks (keyed by their chunk
	// coordinates) but never fills them itself - the first access to a chunk invokes the
	// user-provided generator, which is free to populate the chunk however it pleases.
	// Loaded chunks can see each other through the IVoxelCellLookup interface, so faces
	// laying on chunk boundaries are culled against the neighboring chunk.
	class VoxelMap : public spk::IVoxelCellLookup
	{
		friend class spk::VoxelChunk;

	public:
		using ChunkGenerator = std::function<void(spk::VoxelChunk &)>;

	private:
		const spk::VoxelRegistry *_registry = nullptr;
		ChunkGenerator _generator;
		spk::GameEngine *_engine = nullptr;
		std::unordered_map<spk::Vector3Int, spk::VoxelChunk> _chunks;
		std::shared_ptr<const void> _lifetimeToken = std::make_shared<const int>(0);
		// Owning thread stamped onto every chunk. Empty until the update loop claims the map
		// (see bindMutationThread), so chunks generated during single-threaded setup stay
		// unbound and freely editable; afterwards, cross-thread mutation is caught.
		std::optional<std::thread::id> _mutationThread;
		// Bumped whenever the observable world content changes: a chunk finishes loading or
		// unloads, clear() removes chunks, or a chunk editor commits changed cells (one bump
		// per committed batch, never for identical assignments). Caches such as navigation
		// graphs watch this value, including for edits made directly by engine systems
		// (e.g. the fluid simulation) that bypass application-side write paths.
		std::uint64_t _revision = 0;

		void _requestNeighborSynchronization(const spk::Vector3Int &p_coordinates);
		void _onChunkEdited(const spk::VoxelChunk &p_chunk, std::uint8_t p_changedBoundaries) noexcept;

	public:
		VoxelMap(const spk::VoxelRegistry &p_registry, ChunkGenerator p_generator, spk::GameEngine *p_engine = nullptr);
		~VoxelMap() override;

		VoxelMap(const VoxelMap &) = delete;
		VoxelMap &operator=(const VoxelMap &) = delete;
		VoxelMap(VoxelMap &&) noexcept = delete;
		VoxelMap &operator=(VoxelMap &&) noexcept = delete;

		// Returns the chunk at the given chunk coordinates, generating it on first access.
		[[nodiscard]] spk::VoxelChunk &chunk(const spk::Vector3Int &p_coordinates);
		[[nodiscard]] spk::VoxelChunk *tryChunk(const spk::Vector3Int &p_coordinates) noexcept;
		[[nodiscard]] const spk::VoxelChunk *tryChunk(const spk::Vector3Int &p_coordinates) const noexcept;
		bool unloadChunk(const spk::Vector3Int &p_coordinates);
		bool setChunkActive(const spk::Vector3Int &p_coordinates, bool p_active);
		void clear();

		[[nodiscard]] std::size_t loadedChunkCount() const noexcept;
		[[nodiscard]] std::vector<spk::Vector3Int> loadedChunkCoordinates() const;
		[[nodiscard]] std::weak_ptr<const void> lifetimeToken() const noexcept;

		// World-content revision: moves whenever observable cell content changes (chunk
		// load/unload/clear, committed cell edits). Identical assignments never bump it.
		[[nodiscard]] std::uint64_t revision() const noexcept;

		// Cell access in world coordinates, restricted to already-loaded chunks.
		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const override;
		[[nodiscard]] const spk::VoxelCell *tryRenderableCell(const spk::Vector3Int &p_worldCell) const override;
		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_worldCell) const noexcept;
		bool setCell(const spk::Vector3Int &p_worldCell, const spk::VoxelCell &p_cell);

		// Stamps the prefab into the world: its pivot lands on p_worldDestination and
		// every voxel lands at its position rotated around that pivot (negative-y
		// layers land underneath). Every chunk overlapped by the stamp is generated on
		// demand first, so the prefab always lands whole; affected meshes (including
		// boundary neighbors) are re-baked on the next render pass.
		void applyPrefab(
			const spk::Prefab &p_prefab,
			const spk::Vector3Int &p_worldDestination,
			spk::VoxelOrientation p_orientation = spk::VoxelOrientation::PositiveZ);

		[[nodiscard]] const spk::VoxelRegistry &registry() const noexcept;

		// Binds every current and future chunk's mutation guard to p_thread (the caller by
		// default). VoxelChunkStreamerLogic calls this from the update loop the first time it
		// drives the map, so chunks warmed up on the construction thread stop tripping the
		// guard and later off-thread edits are caught. Idempotent when already bound to p_thread.
		void bindMutationThread(std::thread::id p_thread = std::this_thread::get_id());
		// Clears the guard on the map and every loaded chunk: mutation becomes unrestricted.
		void unbindMutationThread() noexcept;
		[[nodiscard]] bool hasBoundMutationThread() const noexcept;

		template <typename TFunction>
		void forEachLoadedChunk(TFunction p_function)
		{
			for (auto &[coordinates, loadedChunk] : _chunks)
			{
				(void)coordinates;
				p_function(loadedChunk);
			}
		}

		template <typename TFunction>
		void forEachLoadedChunk(TFunction p_function) const
		{
			for (const auto &[coordinates, loadedChunk] : _chunks)
			{
				(void)coordinates;
				p_function(loadedChunk);
			}
		}
	};
}
