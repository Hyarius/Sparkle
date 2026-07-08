#pragma once

#include "structures/voxel/spk_prefab.hpp"

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
		spk::Prefab prefab;
		std::vector<PrefabAnchor> anchors;

		[[nodiscard]] const spk::Vector3Int &size() const noexcept { return prefab.size(); }
	};

	[[nodiscard]] PrefabDefinition parsePrefabDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels);
}
