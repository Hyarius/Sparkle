#include "world/voxel_world.hpp"

#include "world/chunk_provider.hpp"

namespace pg
{
	VoxelWorld::VoxelWorld(const VoxelRegistry &p_registry, spk::GameEngine *p_engine) :
		_registry(&p_registry),
		_map(
			p_registry.renderRegistry(),
			[this](spk::VoxelChunk &p_chunk) {
				if (_provider != nullptr)
				{
					_provider->fill(p_chunk);
				}
			},
			p_engine)
	{
	}

	void VoxelWorld::setProvider(const IChunkProvider *p_provider) noexcept
	{
		_provider = p_provider;
	}

	spk::VoxelMap &VoxelWorld::map() noexcept
	{
		return _map;
	}

	const spk::VoxelMap &VoxelWorld::map() const noexcept
	{
		return _map;
	}

	const VoxelCell *VoxelWorld::tryCell(const spk::Vector3Int &p_worldPosition) const
	{
		return _map.tryCell(p_worldPosition);
	}

	const VoxelDefinition *VoxelWorld::tryDefinition(const VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry->tryGet(p_cell.id);
	}

	const VoxelCell &VoxelWorld::cell(const spk::Vector3Int &p_worldPosition) const
	{
		return _map.cell(p_worldPosition);
	}

	bool VoxelWorld::setCell(const spk::Vector3Int &p_worldPosition, const VoxelCell &p_cell)
	{
		if (_map.setCell(p_worldPosition, p_cell) == false)
		{
			return false;
		}
		++_editRevision;
		return true;
	}

	void VoxelWorld::loadChunk(const ChunkCoordinates &p_coordinates)
	{
		(void)_map.chunk(p_coordinates.value);
	}

	bool VoxelWorld::unloadChunk(const ChunkCoordinates &p_coordinates)
	{
		return _map.unloadChunk(p_coordinates.value);
	}

	void VoxelWorld::clear()
	{
		_map.clear();
	}

	std::size_t VoxelWorld::loadedChunkCount() const noexcept
	{
		return _map.loadedChunkCount();
	}

	std::size_t VoxelWorld::revision() const noexcept
	{
		// Any load/unload changes the chunk count; edits bump the counter explicitly. The
		// navigation graph rebuilds whenever this value moves.
		return _editRevision + _map.loadedChunkCount();
	}

	const VoxelRegistry &VoxelWorld::registry() const noexcept
	{
		return *_registry;
	}

	std::vector<ChunkCoordinates> VoxelWorld::loadedChunkCoordinates() const
	{
		std::vector<ChunkCoordinates> result;
		const std::vector<spk::Vector3Int> coordinates = _map.loadedChunkCoordinates();
		result.reserve(coordinates.size());
		for (const spk::Vector3Int &value : coordinates)
		{
			result.push_back(ChunkCoordinates{value});
		}
		return result;
	}
}
