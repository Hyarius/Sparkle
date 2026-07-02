#include "board/cell_source.hpp"

#include "world/voxel_world.hpp"

namespace
{
	const pg::VoxelCell EmptyCell{};
}

namespace pg
{
	GridCellSource::GridCellSource(const VoxelGrid &p_grid, const VoxelRegistry &p_registry) :
		_grid(p_grid), _registry(p_registry)
	{
	}

	const VoxelCell &GridCellSource::cell(const spk::Vector3Int &p_position) const
	{
		return _grid.isWithinBounds(p_position) ? _grid.cell(p_position) : EmptyCell;
	}

	const VoxelDefinition *GridCellSource::tryDefinition(const VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry.tryGet(p_cell.id);
	}

	WorldCellSource::WorldCellSource(const VoxelWorld &p_world) : _world(p_world)
	{
	}

	const VoxelCell &WorldCellSource::cell(const spk::Vector3Int &p_position) const
	{
		return _world.cell(p_position);
	}

	const VoxelDefinition *WorldCellSource::tryDefinition(const VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _world.registry().tryGet(p_cell.id);
	}

	bool isSolid(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		const VoxelCell &value = p_source.cell(p_position);
		const VoxelDefinition *definition = p_source.tryDefinition(value);
		return definition != nullptr && definition->data.traversal == VoxelTraversal::Solid;
	}

	bool isPassableSpace(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		const VoxelCell &value = p_source.cell(p_position);
		if (value.isEmpty()) return true;
		const VoxelDefinition *definition = p_source.tryDefinition(value);
		return definition != nullptr && definition->data.traversal == VoxelTraversal::Passable;
	}

	bool isStandable(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		return isSolid(p_source, p_position) &&
			   isPassableSpace(p_source, p_position + spk::Vector3Int{0, 1, 0}) &&
			   isPassableSpace(p_source, p_position + spk::Vector3Int{0, 2, 0});
	}
}
