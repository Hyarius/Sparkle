#include "structures/voxel/spk_voxel_map.hpp"

#include "structures/game_engine/spk_game_engine.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace
{
	const spk::VoxelCell EmptyCell{};

	constexpr std::array NeighborOffsets = {
		spk::Vector3Int{-1, 0, 0},
		spk::Vector3Int{1, 0, 0},
		spk::Vector3Int{0, -1, 0},
		spk::Vector3Int{0, 1, 0},
		spk::Vector3Int{0, 0, -1},
		spk::Vector3Int{0, 0, 1}};
}

namespace spk
{
	VoxelMap::VoxelMap(const spk::VoxelRegistry &p_registry, ChunkGenerator p_generator, spk::GameEngine *p_engine) :
		_registry(&p_registry),
		_generator(std::move(p_generator)),
		_engine(p_engine)
	{
		if (_generator == nullptr)
		{
			throw std::invalid_argument("VoxelMap requires a chunk generator");
		}
	}

	VoxelMap::~VoxelMap()
	{
		clear();
	}

	void VoxelMap::_requestNeighborSynchronization(const spk::Vector3Int &p_coordinates)
	{
		for (const spk::Vector3Int &offset : NeighborOffsets)
		{
			if (spk::VoxelChunk *neighbor = tryChunk(p_coordinates + offset); neighbor != nullptr)
			{
				neighbor->renderer().requestSynchronization();
			}
		}
	}

	spk::VoxelChunk &VoxelMap::chunk(const spk::Vector3Int &p_coordinates)
	{
		if (spk::VoxelChunk *existing = tryChunk(p_coordinates); existing != nullptr)
		{
			return *existing;
		}

		// unordered_map is node-based: the chunk's address stays stable even if the
		// generator loads further chunks (and rehashes the table) while filling this one.
		const auto emplaced = _chunks.try_emplace(p_coordinates, p_coordinates, *_registry, this);
		spk::VoxelChunk &result = emplaced.first->second;
		if (_engine != nullptr)
		{
			_engine->addEntity(&result);
		}
		try
		{
			_generator(result);
			result.renderer().requestSynchronization();
		} catch (...)
		{
			if (_engine != nullptr)
			{
				_engine->removeEntity(&result);
			}
			_chunks.erase(emplaced.first);
			_requestNeighborSynchronization(p_coordinates);
			throw;
		}
		_requestNeighborSynchronization(p_coordinates);
		return result;
	}

	spk::VoxelChunk *VoxelMap::tryChunk(const spk::Vector3Int &p_coordinates) noexcept
	{
		const auto iterator = _chunks.find(p_coordinates);
		return iterator == _chunks.end() ? nullptr : &iterator->second;
	}

	const spk::VoxelChunk *VoxelMap::tryChunk(const spk::Vector3Int &p_coordinates) const noexcept
	{
		const auto iterator = _chunks.find(p_coordinates);
		return iterator == _chunks.end() ? nullptr : &iterator->second;
	}

	bool VoxelMap::unloadChunk(const spk::Vector3Int &p_coordinates)
	{
		const auto iterator = _chunks.find(p_coordinates);
		if (iterator == _chunks.end())
		{
			return false;
		}
		if (_engine != nullptr)
		{
			_engine->removeEntity(&iterator->second);
		}
		_chunks.erase(iterator);
		_requestNeighborSynchronization(p_coordinates);
		return true;
	}

	bool VoxelMap::setChunkActive(const spk::Vector3Int &p_coordinates, bool p_active)
	{
		spk::VoxelChunk *target = tryChunk(p_coordinates);
		if (target == nullptr || target->isActivated() == p_active)
		{
			return false;
		}

		if (p_active)
		{
			target->activate();
			target->renderer().requestSynchronization();
		}
		else
		{
			target->deactivate();
		}
		_requestNeighborSynchronization(p_coordinates);
		return true;
	}

	void VoxelMap::clear()
	{
		if (_engine != nullptr)
		{
			for (auto &[coordinates, loadedChunk] : _chunks)
			{
				(void)coordinates;
				_engine->removeEntity(&loadedChunk);
			}
		}
		_chunks.clear();
	}

	std::size_t VoxelMap::loadedChunkCount() const noexcept
	{
		return _chunks.size();
	}

	std::vector<spk::Vector3Int> VoxelMap::loadedChunkCoordinates() const
	{
		std::vector<spk::Vector3Int> result;
		result.reserve(_chunks.size());
		for (const auto &[coordinates, loadedChunk] : _chunks)
		{
			(void)loadedChunk;
			result.push_back(coordinates);
		}
		return result;
	}

	const spk::VoxelCell *VoxelMap::tryCell(const spk::Vector3Int &p_worldCell) const
	{
		const spk::VoxelChunk *loaded = tryChunk(spk::VoxelChunk::coordinatesFromWorldCell(p_worldCell));
		return loaded == nullptr ? nullptr : &loaded->grid().cell(loaded->localFromWorld(p_worldCell));
	}

	const spk::VoxelCell *VoxelMap::tryRenderableCell(const spk::Vector3Int &p_worldCell) const
	{
		const spk::VoxelChunk *loaded = tryChunk(spk::VoxelChunk::coordinatesFromWorldCell(p_worldCell));
		return loaded == nullptr || loaded->isActivated() == false
				   ? nullptr
				   : &loaded->grid().cell(loaded->localFromWorld(p_worldCell));
	}

	const spk::VoxelCell &VoxelMap::cell(const spk::Vector3Int &p_worldCell) const noexcept
	{
		const spk::VoxelCell *result = tryCell(p_worldCell);
		return result == nullptr ? EmptyCell : *result;
	}

	bool VoxelMap::setCell(const spk::Vector3Int &p_worldCell, const spk::VoxelCell &p_cell)
	{
		spk::VoxelChunk *target = tryChunk(spk::VoxelChunk::coordinatesFromWorldCell(p_worldCell));
		if (target == nullptr)
		{
			return false;
		}
		const spk::Vector3Int local = target->localFromWorld(p_worldCell);
		if (target->cell(local) == p_cell)
		{
			return true;
		}
		target->setCell(local, p_cell);

		// A cell laying on a chunk boundary also affects the occlusion of the adjacent
		// chunk: its mesh must be re-baked too.
		const std::array boundaries = {
			std::pair{local.x == 0, spk::Vector3Int{-1, 0, 0}},
			std::pair{local.x == spk::VoxelChunk::Size.x - 1, spk::Vector3Int{1, 0, 0}},
			std::pair{local.y == 0, spk::Vector3Int{0, -1, 0}},
			std::pair{local.y == spk::VoxelChunk::Size.y - 1, spk::Vector3Int{0, 1, 0}},
			std::pair{local.z == 0, spk::Vector3Int{0, 0, -1}},
			std::pair{local.z == spk::VoxelChunk::Size.z - 1, spk::Vector3Int{0, 0, 1}}};
		for (const auto &[touchesBoundary, offset] : boundaries)
		{
			if (touchesBoundary)
			{
				if (spk::VoxelChunk *neighbor = tryChunk(target->coordinates() + offset); neighbor != nullptr)
				{
					neighbor->renderer().requestSynchronization();
				}
			}
		}
		return true;
	}

	const spk::VoxelRegistry &VoxelMap::registry() const noexcept
	{
		return *_registry;
	}
}
