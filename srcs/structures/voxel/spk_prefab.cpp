#include "structures/voxel/spk_prefab.hpp"

#include <algorithm>

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

	// Rotates a pivot-relative position by quarter turns around the +Y axis.
	[[nodiscard]] spk::Vector3Int rotateLocal(const spk::Vector3Int &p_local, int p_turns) noexcept
	{
		switch (p_turns)
		{
		case 1:
			return {p_local.z, p_local.y, -p_local.x};
		case 2:
			return {-p_local.x, p_local.y, -p_local.z};
		case 3:
			return {-p_local.z, p_local.y, p_local.x};
		default:
			return p_local;
		}
	}
}

namespace spk
{
	Prefab::Prefab(const spk::VoxelGrid &p_grid, const spk::Vector3Int &p_origin)
	{
		const spk::Vector3Int gridSize = p_grid.size();
		_voxels.reserve(p_grid.cells().size());
		for (int y = 0; y < gridSize.y; ++y)
		{
			for (int z = 0; z < gridSize.z; ++z)
			{
				for (int x = 0; x < gridSize.x; ++x)
				{
					_voxels.push_back({.position = p_origin + spk::Vector3Int{x, y, z}, .cell = p_grid.cell(x, y, z)});
				}
			}
		}
		if (!_voxels.empty())
		{
			_minBounds = p_origin;
			_maxBounds = p_origin + gridSize - spk::Vector3Int{1, 1, 1};
		}
	}

	void Prefab::_growBounds(const spk::Vector3Int &p_minimum, const spk::Vector3Int &p_maximum) noexcept
	{
		if (_voxels.empty())
		{
			_minBounds = p_minimum;
			_maxBounds = p_maximum;
			return;
		}
		_minBounds = {std::min(_minBounds.x, p_minimum.x), std::min(_minBounds.y, p_minimum.y), std::min(_minBounds.z, p_minimum.z)};
		_maxBounds = {std::max(_maxBounds.x, p_maximum.x), std::max(_maxBounds.y, p_maximum.y), std::max(_maxBounds.z, p_maximum.z)};
	}

	void Prefab::addVoxel(const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell)
	{
		_growBounds(p_position, p_position);
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

		_growBounds(minimum, maximum);
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

	void Prefab::setPivot(const spk::Vector3Int &p_pivot) noexcept
	{
		_pivot = p_pivot;
	}

	const spk::Vector3Int &Prefab::pivot() const noexcept
	{
		return _pivot;
	}

	const spk::Vector3Int &Prefab::minBounds() const noexcept
	{
		return _minBounds;
	}

	const spk::Vector3Int &Prefab::maxBounds() const noexcept
	{
		return _maxBounds;
	}

	spk::Vector3Int Prefab::size() const noexcept
	{
		return _voxels.empty() ? spk::Vector3Int{0, 0, 0} : _maxBounds - _minBounds + spk::Vector3Int{1, 1, 1};
	}

	const std::vector<Prefab::Voxel> &Prefab::voxels() const noexcept
	{
		return _voxels;
	}

	std::pair<spk::Vector3Int, spk::Vector3Int> Prefab::rotatedBounds(spk::VoxelOrientation p_orientation) const noexcept
	{
		if (_voxels.empty())
		{
			return {{0, 0, 0}, {0, 0, 0}};
		}
		const int turns = quarterTurns(p_orientation);
		const spk::Vector3Int a = rotateLocal(_minBounds - _pivot, turns);
		const spk::Vector3Int b = rotateLocal(_maxBounds - _pivot, turns);
		return {
			{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
			{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}};
	}

	void Prefab::forEachAppliedVoxel(
		const spk::Vector3Int &p_destination,
		spk::VoxelOrientation p_orientation,
		const std::function<void(const spk::Vector3Int &, const spk::VoxelCell &)> &p_visitor) const
	{
		if (p_visitor == nullptr)
		{
			return;
		}

		const int turns = quarterTurns(p_orientation);
		for (const Voxel &voxel : _voxels)
		{
			const spk::Vector3Int position = p_destination + rotateLocal(voxel.position - _pivot, turns);
			spk::VoxelCell cell = voxel.cell;
			if (!cell.isEmpty())
			{
				cell.orientation = orientationFromQuarterTurns(quarterTurns(cell.orientation) + turns);
			}
			p_visitor(position, cell);
		}
	}

	void Prefab::applyTo(spk::VoxelGrid &p_grid, const spk::Vector3Int &p_destination, spk::VoxelOrientation p_orientation) const
	{
		forEachAppliedVoxel(p_destination, p_orientation, [&p_grid](const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell) {
			if (!p_grid.isWithinBounds(p_position))
			{
				return;
			}
			p_grid.cell(p_position) = p_cell;
		});
	}
}
