#include "world/generator/stair_planner.hpp"
#include "world/prefab_placement_math.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>

namespace pg
{
	std::optional<StairCandidate> planStair(const StairRequest &p_request, const StairPlanningContext &p_context)
	{
		StairCandidate result{.placements = p_request.placements, .record = p_request.record};
		for (const PrefabPlacement &placement : result.placements)
		{
			const PrefabDefinition *prefab = p_context.prefabs.tryGet(placement.prefabId);
			if (prefab == nullptr) return std::nullopt;
			const ResolvedPlacementBox resolved = resolvePlacement(prefab->prefab, placement);
			const spk::Vector3Int pivot = prefab->prefab.pivot();
			const spk::Vector3Int localMin = prefab->clearance ? prefab->clearance->min : prefab->prefab.minBounds();
			const spk::Vector3Int localMax = prefab->clearance ? prefab->clearance->max : prefab->prefab.maxBounds();
			const int turns = spk::quarterTurnsOf(placement.orientation);
			const spk::Vector3Int a = spk::rotateQuarterTurns(localMin - pivot, turns);
			const spk::Vector3Int b = spk::rotateQuarterTurns(localMax - pivot, turns);
			result.claims.push_back({resolved.destination + spk::Vector3Int{std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z)}, resolved.destination + spk::Vector3Int{std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)}});
		}
		return result;
	}
}
