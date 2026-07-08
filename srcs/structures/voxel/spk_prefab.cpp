#include "structures/voxel/spk_prefab.hpp"

#include <algorithm>
#include <stdexcept>

namespace
{
	// Quarter-turns around +Y, in the order PositiveZ -> PositiveX -> NegativeZ -> NegativeX
	// (the enum value names give the world direction of the shape's local +Z axis).
	[[nodiscard]] int quarterTurns(spk::VoxelOrientation p_orientation) noexcept
	{
		switch (p_orientation)
		{
		case spk::VoxelOrientation::PositiveZ:
			return 0;
		case spk::VoxelOrientation::PositiveX:
			return 1;
		case spk::VoxelOrientation::NegativeZ:
			return 2;
		case spk::VoxelOrientation::NegativeX:
			return 3;
		default:
			return 0;
		}
	}

	[[nodiscard]] spk::VoxelOrientation orientationFromQuarterTurns(int p_turns) noexcept
	{
		switch (((p_turns % 4) + 4) % 4)
		{
		case 1:
			return spk::VoxelOrientation::PositiveX;
		case 2:
			return spk::VoxelOrientation::NegativeZ;
		case 3:
			return spk::VoxelOrientation::NegativeX;
		default:
			return spk::VoxelOrientation::PositiveZ;
		}
	}

	// Rotates a local prefab coordinate into the rotated box (whose min corner stays at
	// the origin); p_size is the prefab's unrotated size.
	[[nodiscard]] spk::Vector3Int rotateLocal(const spk::Vector3Int &p_local, const spk::Vector3Int &p_size, int p_turns) noexcept
	{
		switch (p_turns)
		{
		case 1:
			return {p_local.z, p_local.y, p_size.x - 1 - p_local.x};
		case 2:
			return {p_size.x - 1 - p_local.x, p_local.y, p_size.z - 1 - p_local.z};
		case 3:
			return {p_size.z - 1 - p_local.z, p_local.y, p_local.x};
		default:
			return p_local;
		}
	}
}

namespace spk
{
	Prefab::Prefab(const spk::Vector3Int &p_size) :
		_size(p_size)
	{
		if (p_size.x < 0 || p_size.y < 0 || p_size.z < 0)
		{
			throw std::invalid_argument("Prefab size cannot be negative");
		}
	}

	Prefab::Prefab(const spk::VoxelGrid &p_grid) :
		_size(p_grid.size())
	{
		_voxels.reserve(p_grid.cells().size());
		for (int y = 0; y < _size.y; ++y)
		{
			for (int z = 0; z < _size.z; ++z)
			{
				for (int x = 0; x < _size.x; ++x)
				{
					_voxels.push_back({.position = {x, y, z}, .cell = p_grid.cell(x, y, z)});
				}
			}
		}
	}

	void Prefab::addVoxel(const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell)
	{
		if (p_position.x < 0 || p_position.y < 0 || p_position.z < 0 ||
			p_position.x >= _size.x || p_position.y >= _size.y || p_position.z >= _size.z)
		{
			throw std::out_of_range("Prefab voxel position is out of bounds");
		}
		_voxels.push_back({.position = p_position, .cell = p_cell});
	}

	void Prefab::addVoxelRange(
		const spk::Vector3Int &p_firstPosition,
		const spk::Vector3Int &p_secondPosition,
		const spk::VoxelCell &p_cell)
	{
		const spk::Vector3Int minimum{
			std::min(p_firstPosition.x, p_secondPosition.x),
			std::min(p_firstPosition.y, p_secondPosition.y),
			std::min(p_firstPosition.z, p_secondPosition.z)};
		const spk::Vector3Int maximum{
			std::max(p_firstPosition.x, p_secondPosition.x),
			std::max(p_firstPosition.y, p_secondPosition.y),
			std::max(p_firstPosition.z, p_secondPosition.z)};

		// Validate the complete box before inserting anything, so a rejected range
		// cannot leave a partially modified prefab.
		if (minimum.x < 0 || minimum.y < 0 || minimum.z < 0 ||
			maximum.x >= _size.x || maximum.y >= _size.y || maximum.z >= _size.z)
		{
			throw std::out_of_range("Prefab voxel range is out of bounds");
		}

		for (int y = minimum.y; y <= maximum.y; ++y)
		{
			for (int z = minimum.z; z <= maximum.z; ++z)
			{
				for (int x = minimum.x; x <= maximum.x; ++x)
				{
					_voxels.push_back({.position = {x, y, z}, .cell = p_cell});
				}
			}
		}
	}

	const spk::Vector3Int &Prefab::size() const noexcept
	{
		return _size;
	}

	const std::vector<Prefab::Voxel> &Prefab::voxels() const noexcept
	{
		return _voxels;
	}

	spk::Vector3Int Prefab::rotatedSize(spk::VoxelOrientation p_orientation) const noexcept
	{
		return quarterTurns(p_orientation) % 2 == 0 ? _size : spk::Vector3Int{_size.z, _size.y, _size.x};
	}

	void Prefab::applyTo(spk::VoxelGrid &p_grid, const spk::Vector3Int &p_destination, spk::VoxelOrientation p_orientation) const
	{
		const int turns = quarterTurns(p_orientation);
		for (const Voxel &voxel : _voxels)
		{
			const spk::Vector3Int position = p_destination + rotateLocal(voxel.position, _size, turns);
			if (!p_grid.isWithinBounds(position))
			{
				continue;
			}
			spk::VoxelCell cell = voxel.cell;
			if (!cell.isEmpty())
			{
				cell.orientation = orientationFromQuarterTurns(quarterTurns(cell.orientation) + turns);
			}
			p_grid.cell(position) = cell;
		}
	}
}
