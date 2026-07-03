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
		// Build brand-new meshes rather than mutating the existing ones: a render command
		// that captured the previous shared_ptr keeps drawing it safely on the render thread.
		_renderMesh = std::make_shared<spk::TextureMesh3D>(VoxelMesher::buildRenderMesh(
			_grid, *_registry, *_worldLookup, _coordinates.worldOrigin()));
		_maskMesh = std::make_shared<spk::TextureMesh3D>();
		++_meshRevision;
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

	const spk::TextureMesh3D &Chunk::renderMesh() const noexcept
	{
		return *_renderMesh;
	}

	const spk::TextureMesh3D &Chunk::maskMesh() const noexcept
	{
		return *_maskMesh;
	}

	std::shared_ptr<const spk::TextureMesh3D> Chunk::sharedRenderMesh() const noexcept
	{
		return _renderMesh;
	}

	std::shared_ptr<const spk::TextureMesh3D> Chunk::sharedMaskMesh() const noexcept
	{
		return _maskMesh;
	}

	std::uint64_t Chunk::meshRevision() const noexcept
	{
		return _meshRevision;
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
