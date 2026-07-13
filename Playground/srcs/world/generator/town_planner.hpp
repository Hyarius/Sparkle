#pragma once

#include "world/generator/town_workspace.hpp"
#include "world/prefab_definition.hpp"

#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace pg
{
	enum class TownRejectCategory
	{
		SiteOutOfBounds,
		SiteTerrain,
		MissingContent,
		BuildingPlacement,
		DoorClearance,
		RouteUnavailable,
		RoadWidthConflict,
		StairUnavailable,
		RoadSceneryUnavailable,
		GroundSceneryUnavailable,
		WaterfrontMismatch,
		CommitInvariant
	};

	struct TownRejection
	{
		TownRejectCategory category = TownRejectCategory::CommitInvariant;
		std::string componentId;
		std::optional<WorldColumn> worldColumn;
		int layoutAttempt = 0;
		std::string message;
	};

	class TownPlanningError : public std::runtime_error
	{
	public:
		std::size_t macroEntityIndex = 0;
		std::string compositionId;
		TownRejection rejection;
		std::map<std::string, int> rejectionCounts;
		TownPlanningError(std::size_t p_macroEntityIndex, std::string p_compositionId, TownRejection p_rejection, std::map<std::string, int> p_counts, std::string p_message);
	};

	struct ResolvedTownEntrance
	{
		std::string buildingInstanceId;
		spk::Vector3Int threshold{};
		spk::Vector3Int outward{};
		std::vector<TownColumn> approachColumns;
		TownColumn connectionPoint{};
	};

	struct TownCandidate
	{
		std::string compositionId;
		std::size_t macroEntityIndex = 0;
		PlanEntityKind kind = PlanEntityKind::City;
		TownWorldBounds bounds{};
		int centerRow = 0;
		int centerCol = 0;
		std::vector<std::pair<int, int>> boundaryCells;
		std::vector<PlanPavedColumn> mainRoadSurface;
		std::vector<PrefabPlacement> buildings;
		std::vector<PlanClaim> buildingClaims;
		std::vector<ResolvedTownEntrance> entrances;
		std::vector<PlanPavedColumn> urbanRoadSurface;
		std::vector<UrbanRouteRecord> routes;
		std::vector<PrefabPlacement> roadScenery;
		std::vector<PrefabPlacement> groundScenery;
		std::vector<PlanClaim> sceneryClaims;
		std::vector<std::pair<int, int>> dockCells;
		std::vector<PlanClaim> dockClaims;
		std::optional<spk::Vector3Int> boardingEndpoint;
		std::vector<std::size_t> boatLinkIndices;
	};

	struct TownPlanResult
	{
		std::optional<TownCandidate> candidate;
		std::optional<TownRejection> rejection;
		int layoutAttempts = 0;
		int buildingCandidates = 0;
		int routeExpansions = 0;
	};

	struct TownMutationSnapshot
	{
		std::size_t placements = 0;
		std::size_t stairways = 0;
		std::size_t portals = 0;
		std::size_t towns = 0;
		std::size_t claims = 0;
		std::size_t boatLinks = 0;
		std::size_t warnings = 0;
		friend bool operator==(const TownMutationSnapshot &, const TownMutationSnapshot &) = default;
	};

	[[nodiscard]] TownMutationSnapshot snapshotTownMutation(const WorldPlan &p_plan) noexcept;
	[[nodiscard]] bool matchesTownMutationSnapshot(const WorldPlan &p_plan, TownMutationSnapshot p_snapshot) noexcept;
	[[nodiscard]] TownPlanResult planTown(
		const WorldPlan &p_plan,
		const PlanTownSite &p_site,
		const TownComposition &p_composition,
		const Registry<PrefabDefinition> &p_prefabs,
		const PlanTown &p_biomeTown,
		std::uint64_t p_worldSeed);
	[[nodiscard]] std::string renderTownWorkspaceAscii(const TownWorkspace &p_workspace);
}
