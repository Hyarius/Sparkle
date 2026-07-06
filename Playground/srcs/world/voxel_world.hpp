#pragma once

#include "structures/voxel/spk_voxel_map.hpp"
#include "voxel/voxel_cell.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/chunk_coordinates.hpp"

#include <cstddef>
#include <vector>

namespace spk
{
	class GameEngine;
}

namespace pg
{
	class IChunkProvider;

	// Thin adapter presenting the historical VoxelWorld surface (world-cell + definition
	// lookups, chunk load/unload, revision) on top of the spk voxel map. The map owns the
	// chunks (spk::VoxelChunk entities that bake and stream themselves through the spk
	// render/streamer logics); this class only forwards to it and pairs cells with the
	// gameplay definitions the navigation graph reads.
	class VoxelWorld
	{
	private:
		const VoxelRegistry *_registry = nullptr;
		const IChunkProvider *_provider = nullptr;
		std::size_t _editRevision = 0;
		spk::VoxelMap _map;

	public:
		VoxelWorld(const VoxelRegistry &p_registry, spk::GameEngine *p_engine = nullptr);

		VoxelWorld(const VoxelWorld &) = delete;
		VoxelWorld &operator=(const VoxelWorld &) = delete;

		// The generator the map runs when a chunk is first touched; must outlive the world.
		void setProvider(const IChunkProvider *p_provider) noexcept;

		[[nodiscard]] spk::VoxelMap &map() noexcept;
		[[nodiscard]] const spk::VoxelMap &map() const noexcept;

		[[nodiscard]] const VoxelCell *tryCell(const spk::Vector3Int &p_worldPosition) const;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const;
		[[nodiscard]] const VoxelCell &cell(const spk::Vector3Int &p_worldPosition) const;
		bool setCell(const spk::Vector3Int &p_worldPosition, const VoxelCell &p_cell);

		// Forces a chunk to be generated now (used to warm up the spawn area before the
		// streamer takes over).
		void loadChunk(const ChunkCoordinates &p_coordinates);
		bool unloadChunk(const ChunkCoordinates &p_coordinates);
		void clear();

		[[nodiscard]] std::size_t loadedChunkCount() const noexcept;
		[[nodiscard]] std::size_t revision() const noexcept;
		[[nodiscard]] const VoxelRegistry &registry() const noexcept;
		[[nodiscard]] std::vector<ChunkCoordinates> loadedChunkCoordinates() const;
	};
}
