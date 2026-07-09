#pragma once

#include "core/json.hpp"
#include "voxel/voxel_grid.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	class VoxelRegistry;

	namespace detail
	{
		struct WeightedVoxelCell
		{
			VoxelCell cell;
			double weight = 1.0;
		};

		using VoxelCellPool = std::vector<WeightedVoxelCell>;
		using VoxelPalette = std::map<std::string, VoxelCellPool>;

		[[nodiscard]] spk::Vector3Int parseVector3(const JsonReader &p_reader, const std::string &p_key);
		[[nodiscard]] VoxelOrientation parseOrientation(
			const JsonReader &p_reader,
			const std::string &p_key,
			VoxelOrientation p_default = VoxelOrientation::PositiveZ);
		[[nodiscard]] VoxelFlip parseFlip(
			const JsonReader &p_reader,
			const std::string &p_key,
			VoxelFlip p_default = VoxelFlip::PositiveY);
		[[nodiscard]] VoxelPalette parsePalette(const JsonReader &p_reader, const VoxelRegistry &p_voxels);
		[[nodiscard]] VoxelCell pickPaletteCell(
			const VoxelPalette &p_palette,
			const JsonReader &p_reader,
			const std::string &p_key,
			const spk::Vector3Int &p_position);
		[[nodiscard]] VoxelCell pickPaletteToken(
			const VoxelPalette &p_palette,
			const std::string &p_token,
			const JsonReader &p_reader,
			const std::string &p_path,
			const spk::Vector3Int &p_position);
		// p_offset translates authored coordinates into grid indices — prefabs use it to
		// let content live at negative y (layers embedded below the stamp destination).
		void applyVoxelContent(
			const JsonReader &p_reader,
			VoxelGrid &p_grid,
			const VoxelPalette &p_palette,
			const spk::Vector3Int &p_offset = {0, 0, 0});
	}
}
