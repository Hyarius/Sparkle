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
		_lifetimeToken.reset();
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

	void VoxelMap::_onChunkEdited(const spk::VoxelChunk &p_chunk, std::uint8_t p_changedBoundaries) noexcept
	{
		const std::array boundaries = {
			std::pair{spk::VoxelChunk::NegativeXBoundary, spk::Vector3Int{-1, 0, 0}},
			std::pair{spk::VoxelChunk::PositiveXBoundary, spk::Vector3Int{1, 0, 0}},
			std::pair{spk::VoxelChunk::NegativeYBoundary, spk::Vector3Int{0, -1, 0}},
			std::pair{spk::VoxelChunk::PositiveYBoundary, spk::Vector3Int{0, 1, 0}},
			std::pair{spk::VoxelChunk::NegativeZBoundary, spk::Vector3Int{0, 0, -1}},
			std::pair{spk::VoxelChunk::PositiveZBoundary, spk::Vector3Int{0, 0, 1}}};
		for (const auto &[boundary, offset] : boundaries)
		{
			if ((p_changedBoundaries & boundary) != 0)
			{
				if (spk::VoxelChunk *neighbor = tryChunk(p_chunk.coordinates() + offset); neighbor != nullptr)
				{
					neighbor->renderer().requestSynchronization();
				}
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
		const auto emplaced = _chunks.try_emplace(p_coordinates, p_coordinates, *_registry, this, this, nullptr);
		spk::VoxelChunk &result = emplaced.first->second;
		// Inherit the map's owning thread so the guard covers generation on this very thread and
		// every later edit. While the map is unbound (setup phase) the chunk stays unbound too.
		if (_mutationThread.has_value())
		{
			result.bindMutationThread(*_mutationThread);
		}
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

	std::weak_ptr<const void> VoxelMap::lifetimeToken() const noexcept
	{
		return _lifetimeToken;
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
		return true;
	}

	void VoxelMap::applyPrefab(const spk::Prefab &p_prefab, const spk::Vector3Int &p_worldDestination, spk::VoxelOrientation p_orientation)
	{
		if (p_prefab.voxels().empty())
		{
			return;
		}

		// The rotated bounds are relative to the destination (the pivot lands there and
		// layers may extend below or before it), so the covered box is destination +
		// bounds on every axis.
		const auto [rotatedMin, rotatedMax] = p_prefab.rotatedBounds(p_orientation);
		const spk::Vector3Int minChunk = spk::VoxelChunk::coordinatesFromWorldCell(p_worldDestination + rotatedMin);
		const spk::Vector3Int maxChunk = spk::VoxelChunk::coordinatesFromWorldCell(p_worldDestination + rotatedMax);
		for (int chunkY = minChunk.y; chunkY <= maxChunk.y; ++chunkY)
		{
			for (int chunkZ = minChunk.z; chunkZ <= maxChunk.z; ++chunkZ)
			{
				for (int chunkX = minChunk.x; chunkX <= maxChunk.x; ++chunkX)
				{
					spk::VoxelChunk &target = chunk({chunkX, chunkY, chunkZ});
					target.editCells([&](spk::VoxelChunk::Editor &p_editor) {
						p_editor.applyPrefab(p_prefab, p_worldDestination - target.worldOrigin(), p_orientation);
					});
				}
			}
		}
	}

	const spk::VoxelRegistry &VoxelMap::registry() const noexcept
	{
		return *_registry;
	}

	void VoxelMap::bindMutationThread(std::thread::id p_thread)
	{
		if (_mutationThread.has_value() && *_mutationThread == p_thread)
		{
			return;
		}
		_mutationThread = p_thread;
		for (auto &[coordinates, loadedChunk] : _chunks)
		{
			(void)coordinates;
			loadedChunk.bindMutationThread(p_thread);
		}
	}

	void VoxelMap::unbindMutationThread() noexcept
	{
		_mutationThread.reset();
		for (auto &[coordinates, loadedChunk] : _chunks)
		{
			(void)coordinates;
			loadedChunk.unbindMutationThread();
		}
	}

	bool VoxelMap::hasBoundMutationThread() const noexcept
	{
		return _mutationThread.has_value();
	}
}
