#pragma once

#include "world/generator/world_plan.hpp"

#include "structures/voxel/spk_prefab.hpp"

namespace pg
{
	// Where a prefab placement lands in the world, resolved from the prefab's rotated
	// bounds. Single source of truth shared by the world planner (claimed zones, stair
	// footprints) and the realization (PlanChunkProvider stamping): both must reason
	// about exactly the same voxels, so the formula exists only here.
	struct ResolvedPlacementBox
	{
		spk::Vector3Int worldMin{};	   // min corner of the rotated content box
		spk::Vector3Int extents{};	   // its size per axis
		spk::Vector3Int destination{}; // world cell the prefab's pivot lands on
	};

	// Default placements center the rotated footprint on anchor.x/z and place its
	// lowest authored layer at anchor.y — content below the ground level (floor
	// slabs, POI pedestals) sinks into the terrain. Pivot-anchored placements
	// instead land the prefab's authored pivot exactly on the anchor.
	[[nodiscard]] inline ResolvedPlacementBox resolvePlacement(
		const spk::Prefab &p_prefab,
		const PrefabPlacement &p_placement)
	{
		const auto [rotatedMin, rotatedMax] = p_prefab.rotatedBounds(p_placement.orientation);
		const spk::Vector3Int extents = rotatedMax - rotatedMin + spk::Vector3Int{1, 1, 1};
		const spk::Vector3Int worldMin = p_placement.anchorToPivot
											 ? p_placement.anchor + rotatedMin
											 : spk::Vector3Int{
												   p_placement.anchor.x - extents.x / 2,
												   p_placement.anchor.y + rotatedMin.y,
												   p_placement.anchor.z - extents.z / 2};
		return ResolvedPlacementBox{
			.worldMin = worldMin,
			.extents = extents,
			.destination = p_placement.anchorToPivot ? p_placement.anchor : worldMin - rotatedMin};
	}
}
