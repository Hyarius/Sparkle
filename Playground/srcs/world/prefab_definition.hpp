#pragma once

#include "voxel/voxel_grid.hpp"

#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class VoxelRegistry;

	struct PrefabAnchor
	{
		std::string name;
		spk::Vector3Int at{};
	};

	struct PrefabDefinition
	{
		std::string id;
		VoxelGrid grid;
		std::vector<PrefabAnchor> anchors;

		[[nodiscard]] const spk::Vector3Int &size() const noexcept { return grid.size(); }
	};

	[[nodiscard]] PrefabDefinition parsePrefabDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels);
}
