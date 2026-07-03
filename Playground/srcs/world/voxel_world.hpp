#pragma once

#include "structures/game_engine/spk_entity_3d.hpp"
#include "voxel/voxel_mesher.hpp"
#include "world/chunk.hpp"

#include <map>
#include <memory>
#include <vector>

namespace spk
{
	class GameEngine;
}

namespace pg
{
	class IChunkProvider;
	struct MapDefinition;

	class VoxelWorld final : public ICellLookup
	{
	private:
		struct LoadedChunk
		{
			std::unique_ptr<spk::Entity3D> entity;
			Chunk *chunk = nullptr;
		};

		const VoxelRegistry *_registry = nullptr;
		spk::GameEngine *_engine = nullptr;
		std::map<ChunkCoordinates, LoadedChunk> _chunks;
		std::size_t _revision = 0;

	public:
		VoxelWorld(const VoxelRegistry &p_registry, spk::GameEngine *p_engine = nullptr);
		~VoxelWorld();

		VoxelWorld(const VoxelWorld &) = delete;
		VoxelWorld &operator=(const VoxelWorld &) = delete;

		[[nodiscard]] const VoxelCell *tryCell(const spk::Vector3Int &p_worldPosition) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const override;
		[[nodiscard]] const VoxelCell &cell(const spk::Vector3Int &p_worldPosition) const;
		[[nodiscard]] bool setCell(const spk::Vector3Int &p_worldPosition, const VoxelCell &p_cell);

		[[nodiscard]] Chunk *chunk(const ChunkCoordinates &p_coordinates) noexcept;
		[[nodiscard]] const Chunk *chunk(const ChunkCoordinates &p_coordinates) const noexcept;
		[[nodiscard]] Chunk &loadChunk(const ChunkCoordinates &p_coordinates, const IChunkProvider &p_provider);
		[[nodiscard]] bool unloadChunk(const ChunkCoordinates &p_coordinates);
		void loadFromMap(const MapDefinition &p_map);
		void clear();

		[[nodiscard]] std::size_t loadedChunkCount() const noexcept;
		[[nodiscard]] std::size_t revision() const noexcept;
		[[nodiscard]] const VoxelRegistry &registry() const noexcept;
		[[nodiscard]] std::vector<ChunkCoordinates> loadedChunkCoordinates() const;

		template <typename TFunction>
		void forEachLoadedChunk(TFunction p_function)
		{
			for (auto &[coordinates, loaded] : _chunks)
			{
				(void)coordinates;
				p_function(*loaded.chunk);
			}
		}

		template <typename TFunction>
		void forEachLoadedChunk(TFunction p_function) const
		{
			for (const auto &[coordinates, loaded] : _chunks)
			{
				(void)coordinates;
				p_function(*loaded.chunk);
			}
		}
	};
}
