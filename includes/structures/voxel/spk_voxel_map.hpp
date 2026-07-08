#pragma once

#include "structures/voxel/spk_prefab.hpp"
#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"

#include <functional>
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
	public:
		using ChunkGenerator = std::function<void(spk::VoxelChunk &)>;

	private:
		const spk::VoxelRegistry *_registry = nullptr;
		ChunkGenerator _generator;
		spk::GameEngine *_engine = nullptr;
		std::unordered_map<spk::Vector3Int, spk::VoxelChunk> _chunks;

		void _requestNeighborSynchronization(const spk::Vector3Int &p_coordinates);

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

		// Cell access in world coordinates, restricted to already-loaded chunks.
		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const override;
		[[nodiscard]] const spk::VoxelCell *tryRenderableCell(const spk::Vector3Int &p_worldCell) const override;
		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_worldCell) const noexcept;
		bool setCell(const spk::Vector3Int &p_worldCell, const spk::VoxelCell &p_cell);

		// Stamps the prefab into the world, the min corner of its rotated bounding box
		// landing on p_worldDestination. Every chunk overlapped by that box is generated
		// on demand first, so the prefab always lands whole; affected meshes (including
		// boundary neighbors) are re-baked on the next render pass.
		void applyPrefab(
			const spk::Prefab &p_prefab,
			const spk::Vector3Int &p_worldDestination,
			spk::VoxelOrientation p_orientation = spk::VoxelOrientation::PositiveZ);

		[[nodiscard]] const spk::VoxelRegistry &registry() const noexcept;

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
