#pragma once

#include "voxel/voxel_cell.hpp"

#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace pg
{
	class VoxelGrid
	{
	private:
		spk::Vector3Int _size{};
		std::vector<VoxelCell> _cells;

		[[nodiscard]] std::size_t _index(const spk::Vector3Int &p_position) const
		{
			if (!isWithinBounds(p_position))
			{
				throw std::out_of_range("VoxelGrid cell position is out of bounds");
			}
			return static_cast<std::size_t>(p_position.x) +
				   static_cast<std::size_t>(_size.x) *
					   (static_cast<std::size_t>(p_position.z) +
						static_cast<std::size_t>(_size.z) * static_cast<std::size_t>(p_position.y));
		}

		[[nodiscard]] static std::size_t _cellCount(const spk::Vector3Int &p_size)
		{
			if (p_size.x < 0 || p_size.y < 0 || p_size.z < 0)
			{
				throw std::invalid_argument("VoxelGrid size cannot be negative");
			}

			const std::size_t x = static_cast<std::size_t>(p_size.x);
			const std::size_t y = static_cast<std::size_t>(p_size.y);
			const std::size_t z = static_cast<std::size_t>(p_size.z);
			if (x != 0 && y > std::numeric_limits<std::size_t>::max() / x)
			{
				throw std::length_error("VoxelGrid size is too large");
			}
			const std::size_t xy = x * y;
			if (xy != 0 && z > std::numeric_limits<std::size_t>::max() / xy)
			{
				throw std::length_error("VoxelGrid size is too large");
			}
			return xy * z;
		}

	public:
		VoxelGrid() = default;

		explicit VoxelGrid(const spk::Vector3Int &p_size, const VoxelCell &p_fill = {}) :
			_size(p_size),
			_cells(_cellCount(p_size), p_fill)
		{
		}

		[[nodiscard]] const spk::Vector3Int &size() const noexcept
		{
			return _size;
		}

		[[nodiscard]] bool isWithinBounds(const spk::Vector3Int &p_position) const noexcept
		{
			return p_position.x >= 0 && p_position.y >= 0 && p_position.z >= 0 &&
				   p_position.x < _size.x && p_position.y < _size.y && p_position.z < _size.z;
		}

		[[nodiscard]] bool isWithinBounds(int p_x, int p_y, int p_z) const noexcept
		{
			return isWithinBounds({p_x, p_y, p_z});
		}

		[[nodiscard]] VoxelCell &cell(const spk::Vector3Int &p_position)
		{
			return _cells[_index(p_position)];
		}

		[[nodiscard]] const VoxelCell &cell(const spk::Vector3Int &p_position) const
		{
			return _cells[_index(p_position)];
		}

		[[nodiscard]] VoxelCell &cell(int p_x, int p_y, int p_z)
		{
			return cell({p_x, p_y, p_z});
		}

		[[nodiscard]] const VoxelCell &cell(int p_x, int p_y, int p_z) const
		{
			return cell({p_x, p_y, p_z});
		}

		[[nodiscard]] const std::vector<VoxelCell> &cells() const noexcept
		{
			return _cells;
		}
	};
}
