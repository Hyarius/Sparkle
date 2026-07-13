#pragma once

#include "world/generator/town_blueprint.hpp"
#include "world/generator/world_plan.hpp"
#include "world/generator/stair_planner.hpp"
#include "world/prefab_definition.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace pg
{
	enum class TownRejectCategory { OutOfBounds, Water, TerraceHeight, MissingPrefab, DoorMismatch, ClaimConflict, Disconnected, WaterfrontMismatch };
	struct TownRejection { TownRejectCategory category{}; std::string componentId; LocalCell cell{}; };
	class TownPlanningError : public std::runtime_error
	{
	public:
		std::size_t macroEntityIndex;
		TownRejection rejection;
		TownPlanningError(std::size_t p_macroEntityIndex, TownRejection p_rejection, std::string p_message) : std::runtime_error(std::move(p_message)), macroEntityIndex(p_macroEntityIndex), rejection(std::move(p_rejection)) {}
	};
	struct TownCandidate
	{
		std::string blueprintId;
		int quarterTurns = 0;
		int originRow = 0;
		int originCol = 0;
		PlanEntityKind kind = PlanEntityKind::City;
		std::size_t macroEntityIndex = 0;
		LocalCell entranceCell{};
		std::vector<LocalCell> boundaryCells;
		std::vector<LocalCell> pathCells;
		std::vector<LocalCell> decorationCells;
		std::vector<LocalCell> dockCells;
		std::vector<PrefabPlacement> buildings;
		std::vector<PlanClaim> buildingClaims;
		std::vector<PlanClaim> dockClaims;
		std::vector<StairCandidate> stairs;
		std::optional<spk::Vector3Int> boardingEndpoint;
		std::vector<std::size_t> boatLinkIndices;
	};
	struct TownPlanResult { std::optional<TownCandidate> candidate; std::optional<TownRejection> rejection; };
	struct TownContentContext
	{
		const Registry<PrefabDefinition> &prefabs;
		const PlanTown &town;
		std::uint64_t worldSeed = 0;
		std::string stableEntityId;
		const std::vector<std::string> *roadStairPrefabs = nullptr;
	};
	struct TownMutationSnapshot { std::size_t placements = 0; std::size_t stairways = 0; std::size_t portals = 0; std::size_t towns = 0; std::size_t claims = 0; std::size_t boatLinks = 0; std::size_t townPathCells = 0; std::size_t warnings = 0; friend bool operator==(const TownMutationSnapshot &, const TownMutationSnapshot &) = default; };
	[[nodiscard]] TownMutationSnapshot snapshotTownMutation(const WorldPlan &p_plan) noexcept;
	[[nodiscard]] bool matchesTownMutationSnapshot(const WorldPlan &p_plan, TownMutationSnapshot p_snapshot) noexcept;
	// Stable square-ring enumeration: center first, then each increasing radius
	// clockwise from the north-west corner. Candidate failures never affect order.
	[[nodiscard]] std::vector<LocalCell> enumerateTownOrigins(LocalCell p_center, int p_radius);
	// A resolved town must meet the pre-existing inter-settlement road at its
	// authored entrance.
	[[nodiscard]] bool hasValidGlobalRoadArrival(const WorldPlan &p_plan, const TownCandidate &p_candidate) noexcept;

	// Pure first-stage resolver: it transforms authored masks and validates the
	// dry, level plan-cell envelope without mutating WorldPlan.
	[[nodiscard]] TownPlanResult resolveFlatTownCandidate(const WorldPlan &p_plan, const TownBlueprint &p_blueprint, int p_originRow, int p_originCol, int p_quarterTurns);
	[[nodiscard]] TownPlanResult resolveFlatTownCandidate(const WorldPlan &p_plan, const TownBlueprint &p_blueprint, const TownContentContext &p_content, int p_originRow, int p_originCol, int p_quarterTurns);
}
