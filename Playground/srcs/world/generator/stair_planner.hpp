#pragma once

#include "core/registry.hpp"
#include "world/generator/world_plan.hpp"
#include "world/prefab_definition.hpp"

#include <optional>
#include <vector>

namespace pg
{
	struct StairRequest { std::vector<PrefabPlacement> placements; PlanStairway record; };
	struct StairCandidate { std::vector<PrefabPlacement> placements; std::vector<PlanClaim> claims; PlanStairway record; };
	struct StairPlanningContext { const WorldPlan &plan; const Registry<PrefabDefinition> &prefabs; };

	// Pure shared conversion from an authored stair layout to its exact voxel claims.
	// It owns no references to temporary generator state and never mutates p_context.
	[[nodiscard]] std::optional<StairCandidate> planStair(const StairRequest &p_request, const StairPlanningContext &p_context);
}
