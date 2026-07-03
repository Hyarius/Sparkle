#include "board/board_terrain_layer.hpp"

#include <stdexcept>

namespace pg
{
	BoardTerrainLayer::BoardTerrainLayer(std::unique_ptr<ICellSource> p_cells, spk::Vector3Int p_size) :
		_cells(std::move(p_cells)),
		_size(p_size)
	{
		if (_cells == nullptr)
		{
			throw std::invalid_argument("Board terrain requires a cell source");
		}
		if (_size.x <= 0 || _size.y <= 0 || _size.z <= 0)
		{
			throw std::invalid_argument("Board terrain dimensions must be positive");
		}
	}

	const ICellSource &BoardTerrainLayer::cells() const noexcept
	{
		return *_cells;
	}

	const spk::Vector3Int &BoardTerrainLayer::size() const noexcept
	{
		return _size;
	}

	bool BoardTerrainLayer::isInside(const spk::Vector3Int &p_local) const noexcept
	{
		return p_local.x >= 0 && p_local.y >= 0 && p_local.z >= 0 &&
			   p_local.x < _size.x && p_local.y < _size.y && p_local.z < _size.z;
	}
}
