#include "world/voxel_world.hpp"

#include <utility>

namespace pg
{
	VoxelWorld::VoxelWorld(
		const VoxelRegistry &p_registry,
		spk::VoxelMap::ChunkGenerator p_generator,
		spk::GameEngine *p_engine) :
		_registry(&p_registry),
		_map(p_registry.renderRegistry(), std::move(p_generator), p_engine)
	{
	}

	spk::VoxelMap &VoxelWorld::map() noexcept
	{
		return _map;
	}

	const spk::VoxelMap &VoxelWorld::map() const noexcept
	{
		return _map;
	}

	const spk::VoxelCell *VoxelWorld::tryCell(const spk::Vector3Int &p_worldPosition) const
	{
		return _map.tryCell(p_worldPosition);
	}

	const VoxelDefinition *VoxelWorld::tryDefinition(const spk::VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry->tryDefinition(p_cell.id);
	}

	const spk::VoxelCell &VoxelWorld::cell(const spk::Vector3Int &p_worldPosition) const
	{
		return _map.cell(p_worldPosition);
	}

	bool VoxelWorld::setCell(const spk::Vector3Int &p_worldPosition, const spk::VoxelCell &p_cell)
	{
		return _map.setCell(p_worldPosition, p_cell);
	}

	void VoxelWorld::loadChunk(const spk::Vector3Int &p_coordinates)
	{
		(void)_map.chunk(p_coordinates);
	}

	bool VoxelWorld::unloadChunk(const spk::Vector3Int &p_coordinates)
	{
		return _map.unloadChunk(p_coordinates);
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
		// The map tracks world-content revision itself (chunk load/unload/clear + committed
		// cell edits), so even direct engine edits - e.g. the spk fluid simulation - are
		// observed. The navigation graph rebuilds whenever this value moves.
		return static_cast<std::size_t>(_map.revision());
	}

	const VoxelRegistry &VoxelWorld::registry() const noexcept
	{
		return *_registry;
	}

	std::vector<spk::Vector3Int> VoxelWorld::loadedChunkCoordinates() const
	{
		return _map.loadedChunkCoordinates();
	}
}
