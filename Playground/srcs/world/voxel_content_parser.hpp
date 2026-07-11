#pragma once

#include "core/json.hpp"
#include "core/weighted_pool.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "voxel/voxel_enums.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	class VoxelRegistry;

	namespace detail
	{
		using VoxelCellPool = WeightedPool<spk::VoxelCell>;
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
		[[nodiscard]] spk::VoxelCell pickPaletteCell(
			const VoxelPalette &p_palette,
			const JsonReader &p_reader,
			const std::string &p_key,
			const spk::Vector3Int &p_position);
		[[nodiscard]] spk::VoxelCell pickPaletteToken(
			const VoxelPalette &p_palette,
			const std::string &p_token,
			const JsonReader &p_reader,
			const std::string &p_path,
			const spk::Vector3Int &p_position);
		// p_offset translates authored coordinates into grid indices — prefabs use it to
		// let content live at negative y (layers embedded below the stamp destination).
		void applyVoxelContent(
			const JsonReader &p_reader,
			spk::VoxelGrid &p_grid,
			const VoxelPalette &p_palette,
			const spk::Vector3Int &p_offset = {0, 0, 0});
	}
}
