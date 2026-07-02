#pragma once

#include "core/json.hpp"
#include "voxel/voxel_grid.hpp"

#include <map>
#include <string>

namespace pg
{
	class VoxelRegistry;

	namespace detail
	{
		using VoxelPalette = std::map<std::string, VoxelCell>;

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
		void applyVoxelContent(
			const JsonReader &p_reader,
			VoxelGrid &p_grid,
			const VoxelPalette &p_palette);
	}
}
