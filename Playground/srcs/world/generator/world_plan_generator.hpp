#pragma once

#include "core/registry.hpp"
#include "world/generator/world_plan.hpp"
#include "world/generator/world_plan_math.hpp"
#include "world/interior_definition.hpp"
#include "world/prefab_definition.hpp"
#include "world/prefab_placement_math.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The world-plan generator: one state-owning struct so the stages can share grids
// without dragging parameter lists around. This header is internal to
// world/generator; the stage implementations live in topic files:
//   world_plan_terrain.cpp        stages A-D: graph, landmass + zones, heights, water
//   world_plan_infrastructure.cpp gateways, entities, ports, roads, boats, buildings
//   world_plan_stairways.cpp      claimed zones, stair candidates, stair placement
//   world_plan_interiors.cpp      composed interiors and their portals
//   world_plan_scenery.cpp        biome dressing
//   world_plan_generator.cpp      orchestration, stats, and the public entry point
namespace pg::worldgen
{
	struct Generator
	{
		const WorldGenConfig &cfg;
		const PlanPlacementRules &placementRules;
		const Registry<PrefabDefinition> &prefabs;
		const Registry<InteriorDefinition> &interiors;
		WorldPlan plan;
		int size;

		PlanGrid<int> continents{};
		int continentCount = 0;
		std::map<int, int> zoneContinent;
		std::map<std::pair<int, int>, std::vector<Cell>> borders;
		PlanGrid<int> distOcean{};

		Generator(
			const WorldGenConfig &p_config,
			const std::vector<PlanBiome> &p_biomes,
			const PlanPlacementRules &p_placementRules,
			const Registry<PrefabDefinition> &p_prefabs,
			const Registry<InteriorDefinition> &p_interiors);

		[[nodiscard]] Rng rngFor(const std::string &p_path) const
		{
			return Rng(deriveSeed(cfg.masterSeed, p_path));
		}

		[[nodiscard]] bool isLand(int p_row, int p_col) const
		{
			return plan.land.at(p_row, p_col) != 0;
		}

		// ---------------- Stages A-D (world_plan_terrain.cpp) ----------------
		void buildWorldGraph();
		void buildLandmass();
		void dropTinyIslands();
		[[nodiscard]] std::vector<Cell> landCells() const;
		void assignZones();
		void enforceContiguousZones();
		void mapZoneContinents();
		void findBorders();
		void assignHeights();
		void generateWater();

		// ------- Gateways, entities, ports, roads (world_plan_infrastructure.cpp) -------
		void resolveGateways();
		void placeEntities();
		void assignPorts();

		Rng roadRng{0};

		[[nodiscard]] bool wouldFormRoadSquare(int p_row, int p_col) const;
		[[nodiscard]] double stepCost(int p_fromRow, int p_fromCol, int p_row, int p_col, const Cell &p_goal) const;
		[[nodiscard]] std::vector<Cell> findPath(const Cell &p_start, const Cell &p_goal);
		bool connectRoad(const Cell &p_from, const Cell &p_to);
		[[nodiscard]] std::optional<Cell> nearestRoadCell(int p_row, int p_col, int p_maxRadius = 64) const;
		void buildRoads();
		void removeRoadSquares();
		void addBoatLinks();
		void markBridges();

		// Prefab resolution: the zone's biome pool decides first, then the global
		// placements.json pool; entity kinds without any entry get no prefab. One
		// pool entry is picked at random per placement for diversity.
		Rng prefabPickRng{0};

		[[nodiscard]] const std::string &pickPrefab(const std::vector<std::string> &p_pool);
		[[nodiscard]] std::vector<std::string> stairLengthPoolFor(int p_zone, bool p_onRoad) const;
		std::string pickStairLength(int p_zone, bool p_onRoad);
		[[nodiscard]] std::string stairPlatformPrefabFor(int p_zone, bool p_onRoad) const;
		[[nodiscard]] const std::vector<std::string> *entityPrefabsFor(PlanEntityKind p_kind, int p_zone) const;

		// ---------------- Claimed zones (world_plan_stairways.cpp) ----------------
		// Every structural placement (stairways first — they have priority — then
		// buildings, then interior rooms) claims a world-space box: its authored
		// "clearance" zone when the prefab declares one, its content bounds otherwise.
		// Later placements only commit if their own claimed zone is entirely free, so
		// stairs can no longer merge into POIs and vice versa.
		struct Claim
		{
			spk::Vector3Int min{};
			spk::Vector3Int max{};
		};
		std::vector<Claim> hardClaims;

		[[nodiscard]] std::optional<ResolvedPlacementBox> resolveBox(const PrefabPlacement &p_placement) const;
		[[nodiscard]] std::optional<Claim> claimBoxFor(const PrefabPlacement &p_placement) const;
		[[nodiscard]] static bool claimsOverlap(const Claim &p_first, const Claim &p_second);
		[[nodiscard]] bool zoneIsFree(const Claim &p_claim) const;

		// ---------------- Stairways (world_plan_stairways.cpp) ----------------
		// Lower cells of every stairway placed so far (road + wild), for spacing.
		std::vector<Cell> stairCells;

		struct StairFootprint
		{
			int minX = 0;
			int maxX = 0;
			int minZ = 0;
			int maxZ = 0;
		};

		// Maps the local cliff coordinates of a stair layout ("across" the cliff /
		// "along" its wall) onto world x/z.
		struct CliffFrame
		{
			bool acrossIsX = false;
			const WorldPlan &plan;

			[[nodiscard]] spk::Vector3Int at(int p_across, int p_y, int p_along) const
			{
				return acrossIsX ? spk::Vector3Int{p_across, p_y, p_along}
								 : spk::Vector3Int{p_along, p_y, p_across};
			}

			[[nodiscard]] StairFootprint rect(
				int p_acrossMin,
				int p_acrossMax,
				int p_alongMin,
				int p_alongMax) const
			{
				return acrossIsX ? StairFootprint{p_acrossMin, p_acrossMax, p_alongMin, p_alongMax}
								 : StairFootprint{p_alongMin, p_alongMax, p_acrossMin, p_acrossMax};
			}

			[[nodiscard]] Claim claim(
				int p_acrossMin,
				int p_acrossMax,
				int p_yMin,
				int p_yMax,
				int p_alongMin,
				int p_alongMax) const
			{
				return {at(p_acrossMin, p_yMin, p_alongMin), at(p_acrossMax, p_yMax, p_alongMax)};
			}

			[[nodiscard]] Cell cell(int p_across, int p_along) const
			{
				return acrossIsX
						   ? Cell{plan.cellIndexFromWorld(p_along), plan.cellIndexFromWorld(p_across)}
						   : Cell{plan.cellIndexFromWorld(p_across), plan.cellIndexFromWorld(p_along)};
			}

			[[nodiscard]] spk::VoxelOrientation alongAscend(int p_tangent) const
			{
				if (acrossIsX)
				{
					return p_tangent == 1 ? spk::VoxelOrientation::NegativeZ : spk::VoxelOrientation::PositiveZ;
				}
				return p_tangent == 1 ? spk::VoxelOrientation::NegativeX : spk::VoxelOrientation::PositiveX;
			}

			[[nodiscard]] spk::VoxelOrientation acrossAscend(bool p_positive) const
			{
				if (acrossIsX)
				{
					return p_positive ? spk::VoxelOrientation::PositiveX : spk::VoxelOrientation::NegativeX;
				}
				return p_positive ? spk::VoxelOrientation::PositiveZ : spk::VoxelOrientation::NegativeZ;
			}
		};
		std::vector<StairFootprint> stairFootprints;

		[[nodiscard]] std::optional<StairFootprint> stairFootprintOf(const PrefabPlacement &p_placement) const;
		[[nodiscard]] static bool footprintsOverlap(const StairFootprint &p_first, const StairFootprint &p_second);
		[[nodiscard]] bool planCellHasEntity(int p_row, int p_col) const;

		// Road interaction of a stair footprint: straight ramps crossing on the road
		// network must stay on road cells, composed climbs may also use clear
		// terrain beside the road, and wild stairways must not touch roads at all.
		enum class RoadRule
		{
			Require,
			Allow,
			Forbid
		};

		[[nodiscard]] bool stairFootprintFits(
			const StairFootprint &p_footprint,
			int p_lowLevel,
			int p_lowZone,
			RoadRule p_roadRule,
			const std::vector<StairFootprint> &p_group) const;

		// Geometry of one cliff crossing, shared by every stair layout builder: the
		// local frame, the strata on both sides, and the wall-hugging low-side
		// columns. "Across" is the horizontal axis perpendicular to the cliff,
		// "along" the axis running beside it.
		struct StairSite
		{
			CliffFrame frame;
			int steps = 0;
			int lowLevel = 0;
			int highLevel = 0;
			int lowZone = 0;
			int lowRow = 0;
			int lowCol = 0;
			int surface = 0;		 // stand base of the low ground
			int highSurface = 0;	 // stand base of the high plateau
			int boundary = 0;		 // across coordinate of the first column of the neighbor cell
			int wallSide = 0;		 // across direction pointing away from the wall, onto the low side
			int wallColumn = 0;		 // low-side column hugging the cliff wall
			int alongCenterBase = 0; // along coordinate of the crossing's road-strip center
			bool firstLower = false; // the scanned cell (not its neighbor) is the low side
			bool onRoad = false;
		};

		// One fully shaped staircase attempt from a layout builder, ready for the
		// single validate-and-commit step. checkedBand is validated exactly like a
		// stamped footprint but never realized (the walkway lane beside a road
		// climb); reservedExit is only tested against other stairways.
		struct StairCandidate
		{
			std::vector<PrefabPlacement> placements;
			std::optional<StairFootprint> checkedBand;
			std::optional<StairFootprint> reservedExit;
			std::vector<Claim> claims;
			PlanStairway record;
		};

		bool tryCommitStairCandidate(const StairSite &p_site, StairCandidate &&p_candidate);
		[[nodiscard]] bool guardComposedCandidate(
			const StairSite &p_site,
			StairCandidate &p_candidate,
			const StairFootprint &p_band,
			int p_alongMin,
			int p_alongMax,
			int p_topAlong);
		[[nodiscard]] std::optional<StairCandidate> makeOnePassCandidate(
			const StairSite &p_site,
			const std::string &p_platformId,
			int p_tangent,
			int p_crossOffset);
		[[nodiscard]] std::optional<StairCandidate> makeSwitchbackCandidate(
			const StairSite &p_site,
			const std::string &p_platformId,
			int p_tangent,
			int p_crossOffset);
		[[nodiscard]] StairCandidate makePerpendicularCandidate(const StairSite &p_site);
		int emitStairChain(
			int p_row,
			int p_col,
			int p_dr,
			int p_dc,
			bool p_onRoad,
			std::optional<int> p_wildMaximumLevels = std::nullopt);
		void placeStairways();
		void placeWildStairways();

		// ---------------- Interiors (world_plan_interiors.cpp) ----------------
		int interiorSlotsUsed = 0;

		[[nodiscard]] static std::optional<spk::Vector3Int> connectorDirection(const std::string &p_name);
		void composeInterior(const PrefabPlacement &p_buildingPlacement);

		// ---------------- Buildings (world_plan_infrastructure.cpp) ----------------
		void placeBuildings();

		// ---------------- Scenery (world_plan_scenery.cpp) ----------------
		void placeScenery();

		// ---------------- Orchestration (world_plan_generator.cpp) ----------------
		void computeStats();
		[[nodiscard]] WorldPlan run() &&;
	};
}
