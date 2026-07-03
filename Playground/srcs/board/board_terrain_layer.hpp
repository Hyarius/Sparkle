#pragma once

#include "board/cell_source.hpp"

#include <memory>

namespace pg
{
	class BoardTerrainLayer
	{
	private:
		std::unique_ptr<ICellSource> _cells;
		spk::Vector3Int _size{};

	public:
		BoardTerrainLayer(std::unique_ptr<ICellSource> p_cells, spk::Vector3Int p_size);

		BoardTerrainLayer(BoardTerrainLayer &&) noexcept = default;
		BoardTerrainLayer &operator=(BoardTerrainLayer &&) noexcept = default;
		BoardTerrainLayer(const BoardTerrainLayer &) = delete;
		BoardTerrainLayer &operator=(const BoardTerrainLayer &) = delete;

		[[nodiscard]] const ICellSource &cells() const noexcept;
		[[nodiscard]] const spk::Vector3Int &size() const noexcept;
		[[nodiscard]] bool isInside(const spk::Vector3Int &p_local) const noexcept;
	};
}
