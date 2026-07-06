#include "structures/voxel/spk_voxel_chunk.hpp"

namespace
{
	[[nodiscard]] constexpr int floorDivide(int p_value, int p_divisor) noexcept
	{
		const int quotient = p_value / p_divisor;
		const int remainder = p_value % p_divisor;
		return remainder < 0 ? quotient - 1 : quotient;
	}
}

namespace spk
{
	VoxelChunk::VoxelChunk(
		const spk::Vector3Int &p_coordinates,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		spk::Entity *p_parent) :
		spk::Entity3D(p_parent),
		_coordinates(p_coordinates)
	{
		transform().setPosition(spk::Vector3(worldOrigin()));
		_renderer = &addComponent<spk::VoxelChunkRenderer>(_grid, p_registry, p_worldLookup, worldOrigin());
	}

	spk::Vector3Int VoxelChunk::coordinatesFromWorldCell(const spk::Vector3Int &p_worldCell) noexcept
	{
		return {
			floorDivide(p_worldCell.x, Size.x),
			floorDivide(p_worldCell.y, Size.y),
			floorDivide(p_worldCell.z, Size.z)};
	}

	const spk::Vector3Int &VoxelChunk::coordinates() const noexcept
	{
		return _coordinates;
	}

	spk::Vector3Int VoxelChunk::worldOrigin() const noexcept
	{
		return _coordinates * Size;
	}

	spk::Vector3Int VoxelChunk::localFromWorld(const spk::Vector3Int &p_worldCell) const noexcept
	{
		return p_worldCell - worldOrigin();
	}

	spk::VoxelGrid &VoxelChunk::grid() noexcept
	{
		_renderer->requestSynchronization();
		return _grid;
	}

	const spk::VoxelGrid &VoxelChunk::grid() const noexcept
	{
		return _grid;
	}

	const spk::VoxelCell &VoxelChunk::cell(const spk::Vector3Int &p_localPosition) const
	{
		return _grid.cell(p_localPosition);
	}

	void VoxelChunk::setCell(const spk::Vector3Int &p_localPosition, const spk::VoxelCell &p_cell)
	{
		spk::VoxelCell &current = _grid.cell(p_localPosition);
		if (current == p_cell)
		{
			return;
		}
		current = p_cell;
		_renderer->requestSynchronization();
	}

	spk::VoxelChunkRenderer &VoxelChunk::renderer() noexcept
	{
		return *_renderer;
	}

	const spk::VoxelChunkRenderer &VoxelChunk::renderer() const noexcept
	{
		return *_renderer;
	}
}
