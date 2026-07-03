#include "world/voxel_world.hpp"

#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "world/chunk_provider.hpp"
#include "world/map_definition.hpp"

#include <array>

namespace
{
	const pg::VoxelCell EmptyCell{};
}

namespace pg
{
	VoxelWorld::VoxelWorld(const VoxelRegistry &p_registry, spk::GameEngine *p_engine) :
		_registry(&p_registry),
		_engine(p_engine)
	{
	}

	VoxelWorld::~VoxelWorld()
	{
		clear();
	}

	const VoxelCell *VoxelWorld::tryCell(const spk::Vector3Int &p_worldPosition) const
	{
		const ChunkCoordinates coordinates = ChunkCoordinates::fromWorldCell(p_worldPosition);
		const Chunk *loaded = chunk(coordinates);
		return loaded == nullptr ? nullptr : &loaded->grid().cell(coordinates.localFromWorld(p_worldPosition));
	}

	const VoxelDefinition *VoxelWorld::tryDefinition(const VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry->tryGet(p_cell.id);
	}

	const VoxelCell &VoxelWorld::cell(const spk::Vector3Int &p_worldPosition) const
	{
		const VoxelCell *result = tryCell(p_worldPosition);
		return result == nullptr ? EmptyCell : *result;
	}

	bool VoxelWorld::setCell(const spk::Vector3Int &p_worldPosition, const VoxelCell &p_cell)
	{
		const ChunkCoordinates coordinates = ChunkCoordinates::fromWorldCell(p_worldPosition);
		Chunk *target = chunk(coordinates);
		if (target == nullptr)
		{
			return false;
		}
		const spk::Vector3Int local = coordinates.localFromWorld(p_worldPosition);
		if (target->grid().cell(local) == p_cell)
		{
			return true;
		}
		target->setCell(local, p_cell);
		++_revision;

		const std::array boundaries = {
			std::pair{local.x == 0, spk::Vector3Int{-1, 0, 0}},
			std::pair{local.x == Chunk::Size.x - 1, spk::Vector3Int{1, 0, 0}},
			std::pair{local.y == 0, spk::Vector3Int{0, -1, 0}},
			std::pair{local.y == Chunk::Size.y - 1, spk::Vector3Int{0, 1, 0}},
			std::pair{local.z == 0, spk::Vector3Int{0, 0, -1}},
			std::pair{local.z == Chunk::Size.z - 1, spk::Vector3Int{0, 0, 1}}};
		for (const auto &[touchesBoundary, offset] : boundaries)
		{
			if (touchesBoundary)
			{
				if (Chunk *neighbor = chunk({coordinates.value + offset}); neighbor != nullptr)
				{
					neighbor->requestSynchronization();
				}
			}
		}
		return true;
	}

	Chunk *VoxelWorld::chunk(const ChunkCoordinates &p_coordinates) noexcept
	{
		const auto iterator = _chunks.find(p_coordinates);
		return iterator == _chunks.end() ? nullptr : iterator->second.chunk;
	}

	const Chunk *VoxelWorld::chunk(const ChunkCoordinates &p_coordinates) const noexcept
	{
		const auto iterator = _chunks.find(p_coordinates);
		return iterator == _chunks.end() ? nullptr : iterator->second.chunk;
	}

	Chunk &VoxelWorld::loadChunk(const ChunkCoordinates &p_coordinates, const IChunkProvider &p_provider)
	{
		if (Chunk *existing = chunk(p_coordinates); existing != nullptr)
		{
			return *existing;
		}

		auto entity = std::make_unique<spk::Entity3D>();
		entity->transform().setPosition(spk::Vector3(p_coordinates.worldOrigin()));
		Chunk &loaded = entity->addComponent<Chunk>(p_coordinates, *_registry, *this);
		p_provider.fill(loaded);
		spk::Entity3D *entityPointer = entity.get();
		_chunks.emplace(p_coordinates, LoadedChunk{.entity = std::move(entity), .chunk = &loaded});
		++_revision;
		if (_engine != nullptr)
		{
			_engine->addEntity(entityPointer);
		}

		const std::array offsets = {
			spk::Vector3Int{-1, 0, 0}, spk::Vector3Int{1, 0, 0}, spk::Vector3Int{0, -1, 0}, spk::Vector3Int{0, 1, 0}, spk::Vector3Int{0, 0, -1}, spk::Vector3Int{0, 0, 1}};
		for (const spk::Vector3Int &offset : offsets)
		{
			if (Chunk *neighbor = chunk({p_coordinates.value + offset}); neighbor != nullptr)
			{
				neighbor->requestSynchronization();
			}
		}
		return loaded;
	}

	bool VoxelWorld::unloadChunk(const ChunkCoordinates &p_coordinates)
	{
		auto iterator = _chunks.find(p_coordinates);
		if (iterator == _chunks.end())
		{
			return false;
		}
		if (_engine != nullptr)
		{
			_engine->removeEntity(iterator->second.entity.get());
		}
		_chunks.erase(iterator);
		++_revision;

		const std::array offsets = {
			spk::Vector3Int{-1, 0, 0}, spk::Vector3Int{1, 0, 0}, spk::Vector3Int{0, -1, 0}, spk::Vector3Int{0, 1, 0}, spk::Vector3Int{0, 0, -1}, spk::Vector3Int{0, 0, 1}};
		for (const spk::Vector3Int &offset : offsets)
		{
			if (Chunk *neighbor = chunk({p_coordinates.value + offset}); neighbor != nullptr)
			{
				neighbor->requestSynchronization();
			}
		}
		return true;
	}

	void VoxelWorld::loadFromMap(const MapDefinition &p_map)
	{
		clear();
		MapChunkProvider provider(p_map);
		const spk::Vector3Int chunkCount{
			(p_map.size().x + Chunk::Size.x - 1) / Chunk::Size.x,
			(p_map.size().y + Chunk::Size.y - 1) / Chunk::Size.y,
			(p_map.size().z + Chunk::Size.z - 1) / Chunk::Size.z};
		for (int y = 0; y < chunkCount.y; ++y)
		{
			for (int z = 0; z < chunkCount.z; ++z)
			{
				for (int x = 0; x < chunkCount.x; ++x)
				{
					(void)loadChunk({{x, y, z}}, provider);
				}
			}
		}
	}

	void VoxelWorld::clear()
	{
		if (_engine != nullptr)
		{
			for (auto &[coordinates, loaded] : _chunks)
			{
				(void)coordinates;
				_engine->removeEntity(loaded.entity.get());
			}
		}
		_chunks.clear();
		++_revision;
	}

	std::size_t VoxelWorld::loadedChunkCount() const noexcept
	{
		return _chunks.size();
	}

	std::size_t VoxelWorld::revision() const noexcept
	{
		return _revision;
	}

	const VoxelRegistry &VoxelWorld::registry() const noexcept
	{
		return *_registry;
	}

	std::vector<ChunkCoordinates> VoxelWorld::loadedChunkCoordinates() const
	{
		std::vector<ChunkCoordinates> result;
		result.reserve(_chunks.size());
		for (const auto &[coordinates, unused] : _chunks)
		{
			(void)unused;
			result.push_back(coordinates);
		}
		return result;
	}
}
