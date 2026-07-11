#pragma once

#include "core/json.hpp"
#include "structures/voxel/spk_prefab.hpp"

#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class VoxelRegistry;

	struct PrefabAnchor
	{
		std::string name;
		spk::Vector3Int at{};
	};

	// The zone (inclusive corners, prefab-local like the content) a placement claims for
	// itself: the world planner refuses to stamp two claimed zones on top of each other.
	// Authored in JSON to demand more room than the voxels (a free ring around a house);
	// prefabs without one claim exactly their content bounds.
	struct PrefabClearance
	{
		spk::Vector3Int min{};
		spk::Vector3Int max{};
	};

	struct PrefabDefinition
	{
		std::string id;
		spk::Prefab prefab;
		std::vector<PrefabAnchor> anchors;
		std::optional<PrefabClearance> clearance;
		// Interior definition composed and linked (through a door portal) when the world
		// planner places this prefab; empty for scenery and doorless structures.
		std::string interiorId;

		[[nodiscard]] spk::Vector3Int size() const noexcept { return prefab.size(); }
		[[nodiscard]] const PrefabAnchor *tryAnchor(const std::string &p_name) const noexcept
		{
			for (const PrefabAnchor &anchor : anchors)
			{
				if (anchor.name == p_name)
				{
					return &anchor;
				}
			}
			return nullptr;
		}
	};

	[[nodiscard]] PrefabDefinition parsePrefabDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels);
}
