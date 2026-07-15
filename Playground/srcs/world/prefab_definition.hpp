#pragma once

#include "core/json.hpp"
#include "structures/voxel/spk_prefab.hpp"

#include <optional>
#include <string>

namespace pg
{
	class VoxelRegistry;

	using PrefabAnchor = spk::Prefab::Anchor;

	// The zone (inclusive corners, prefab-local like the content) a placement claims for
	// itself: the world planner refuses to stamp two claimed zones on top of each other.
	// Authored in JSON to demand more room than the voxels (a free ring around a house);
	// prefabs without one claim exactly their content bounds.
	struct PrefabClearance
	{
		spk::Vector3Int min{};
		spk::Vector3Int max{};
	};

	// Town content uses this explicit, planner-facing entrance contract.  The
	// composition planner never derives one from a generic anchor name.
	struct PrefabEntrance
	{
		std::string anchorName = "door";
		spk::VoxelOrientation outwardFacing = spk::VoxelOrientation::NegativeZ;
		PrefabClearance clearApproach{};
	};

	struct PrefabDefinition
	{
		std::string id;
		spk::Prefab prefab;
		std::optional<PrefabClearance> clearance;
		std::optional<PrefabEntrance> entrance;
		// Interior definition composed and linked (through a door portal) when the world
		// planner places this prefab; empty for scenery and doorless structures.
		std::string interiorId;

		[[nodiscard]] spk::Vector3Int size() const noexcept
		{
			return prefab.size();
		}
		[[nodiscard]] const PrefabAnchor *tryAnchor(const std::string &p_name) const noexcept
		{
			return prefab.tryAnchor(p_name);
		}
	};

	[[nodiscard]] PrefabDefinition parsePrefabDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels);
}
