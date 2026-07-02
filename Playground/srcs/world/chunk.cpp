#include "world/chunk.hpp"

#include "voxel/voxel_mesher.hpp"

namespace pg
{
	Chunk::Chunk(
		ChunkCoordinates p_coordinates,
		const VoxelRegistry &p_registry,
		const ICellLookup &p_worldLookup) :
		_coordinates(p_coordinates),
		_registry(&p_registry),
		_worldLookup(&p_worldLookup)
	{
	}

	void Chunk::_synchronize() const
	{
		_renderMesh = VoxelMesher::buildRenderMesh(
			_grid, *_registry, *_worldLookup, _coordinates.worldOrigin());
		_maskMesh.clear();
	}

	const ChunkCoordinates &Chunk::coordinates() const noexcept
	{
		return _coordinates;
	}

	VoxelGrid &Chunk::grid() noexcept
	{
		return _grid;
	}

	const VoxelGrid &Chunk::grid() const noexcept
	{
		return _grid;
	}

	const Mesh3D &Chunk::renderMesh() const noexcept
	{
		return _renderMesh;
	}

	const Mesh3D &Chunk::maskMesh() const noexcept
	{
		return _maskMesh;
	}

	void Chunk::setCell(const spk::Vector3Int &p_localPosition, const VoxelCell &p_cell)
	{
		VoxelCell &current = _grid.cell(p_localPosition);
		if (current == p_cell)
		{
			return;
		}
		current = p_cell;
		requestSynchronization();
	}
}
