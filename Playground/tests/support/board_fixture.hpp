#pragma once

#include "board/board_builder.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pg::test
{
	class BoardFixture
	{
	private:
		VoxelGrid _grid;
		std::unique_ptr<BoardData> _board;
		std::map<std::string, spk::Vector3Int> _namedCells;

	public:
		explicit BoardFixture(
			const std::vector<std::string> &p_rows,
			std::map<std::string, spk::Vector3Int> p_namedCells = {});
		BoardFixture(
			const std::vector<std::vector<std::string>> &p_layers,
			std::map<std::string, spk::Vector3Int> p_namedCells = {});

		[[nodiscard]] static const VoxelRegistry &registry();
		[[nodiscard]] const VoxelGrid &grid() const noexcept;
		[[nodiscard]] BoardData &board() noexcept;
		[[nodiscard]] const BoardData &board() const noexcept;
		[[nodiscard]] const TraversalGraph &graph() const noexcept;
		[[nodiscard]] spk::Vector3Int cell(int p_x, int p_z) const;
		[[nodiscard]] spk::Vector3Int cell(const std::string &p_name) const;
		void name(std::string p_name, const spk::Vector3Int &p_cell);
	};
}
