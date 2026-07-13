#pragma once

#include "core/registry.hpp"
#include "structures/container/spk_grid_2d.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"
#include "world/generator/town_composition.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace pg
{
	// Square per-plan-cell grid. A plan cell is the macro unit of the world skeleton
	// (one cell = blocksPerCell x blocksPerCell voxel columns once realized).
	template <typename TValue>
	class PlanGrid
	{
	public:
		static constexpr int MaximumDimension = 4096;

	private:
		int _size = 0;
		spk::Grid2D<TValue> _values;

		[[nodiscard]] static spk::Vector2UInt _checkedSize(int p_size)
		{
			if (p_size <= 0)
			{
				throw std::invalid_argument("PlanGrid size must be positive");
			}
			if (p_size > MaximumDimension)
			{
				throw std::length_error("PlanGrid size exceeds MaximumDimension");
			}
			const unsigned int side = static_cast<unsigned int>(p_size);
			return {side, side};
		}

	public:
		PlanGrid() = default;
		explicit PlanGrid(int p_size, TValue p_fill = TValue{}) :
			_size(p_size),
			_values(_checkedSize(p_size), p_fill)
		{
		}

		[[nodiscard]] int size() const noexcept
		{
			return _size;
		}
		[[nodiscard]] bool contains(int p_row, int p_col) const noexcept
		{
			return p_row >= 0 && p_col >= 0 && p_row < _size && p_col < _size;
		}
		[[nodiscard]] TValue &at(int p_row, int p_col)
		{
			if (!contains(p_row, p_col))
			{
				throw std::out_of_range("PlanGrid coordinates are outside the grid");
			}
			return _values.at(static_cast<std::size_t>(p_col), static_cast<std::size_t>(p_row));
		}
		[[nodiscard]] const TValue &at(int p_row, int p_col) const
		{
			if (!contains(p_row, p_col))
			{
				throw std::out_of_range("PlanGrid coordinates are outside the grid");
			}
			return _values.at(static_cast<std::size_t>(p_col), static_cast<std::size_t>(p_row));
		}
	};

	enum class PlanEntityKind : std::uint8_t
	{
		Gym,
		City,
		PortCity,
		RarePoi,
		UncommonPoi,
		NormalPoi
	};

	struct PlanScenery
	{
		std::string prefabId;
		double density = 0.0; // expected instances per suitable world-plan cell
		int spacing = 1;	  // minimum center distance in voxel columns
		spk::Vector3Int prefabSize{};
		bool roadside = false;
	};

	struct PlanTown
	{
		std::string creatureCenter;
		std::string shop;
		std::string gym;
		std::string port;
		std::vector<std::string> homes;
	};

	// One world-generation biome, instantiated from a biome definition JSON
	// (resources/data/biomes/<id>.json) that carries a "worldgen" block. Definitions
	// without that block (e.g. interior biomes like caves) never enter world generation.
	struct PlanBiome
	{
		std::string id; // biome definition id, resolves palettes at realization
		std::string displayName;
		double heightShift = 0.0;			// strata-level bias for zones of this biome
		bool peak = false;					// hosts summits and takes the full peak lift
		std::optional<spk::Color> mapColor; // zone fill on the preview map (absent = auto)
		// Per-biome entity prefab pools; the generator picks one entry at random per
		// placement.
		std::map<PlanEntityKind, std::vector<std::string>> entityPrefabs;
		std::optional<PlanTown> town;
		// Required per-biome settlement distribution.  The area divided by this
		// spacing squared determines the total number of settlements in a zone.
		double townDistanceCells = 0.0;
		bool requiresPort = false;
		// Decorative structures scattered on clear land in this biome. Unlike POIs these
		// have no gameplay role and may be multi-voxel prefabs such as trees or plants.
		std::vector<PlanScenery> scenery;
		std::vector<PlanScenery> townScenery;
		bool wildStairsConfigured = false;
		std::optional<bool> wildStairAllowCrossZone;
		std::optional<int> wildStairsMaxPerZone;
		std::optional<int> wildStairMaxLevels;
		std::optional<double> wildStairSpacingCells;
		std::optional<double> wildStairCandidateRatio;
		// Climb prefabs synthesized from the biome's stair/slope voxels: the flight pools
		// hold pre-mixed variants (one picked per staircase segment); the platform ids are
		// the single road/surface pads. Empty pools fall back to the shared staircase.
		std::vector<std::string> roadStairLengths;
		std::string roadStairPlatform;
		std::vector<std::string> wildSlopeLengths;
		std::string wildSlopePlatform;
	};

	struct PlanZone
	{
		int id = 0;
		int biomeIndex = 0; // index into WorldPlan::biomes
		int progression = 0;
	};

	struct PlanEntity
	{
		PlanEntityKind kind = PlanEntityKind::City;
		int row = 0;
		int col = 0;
		int zone = -1;
		int continent = 0;
	};

	struct PlanGateway
	{
		int zoneA = 0;
		int zoneB = 0;
		int row = 0;
		int col = 0;
		bool primary = true;
	};

	// A prefab stamped into the realized voxel world at a fixed spot. anchor.x/z is the
	// center column of the prefab's rotated footprint, anchor.y its ground level (the
	// first air layer above the terrain surface) — prefab content at negative local y
	// sinks below it, replacing the terrain there (floor slabs, pedestals); orientation
	// tells where the prefab's local +Z axis points. Foundation placements grow a solid
	// pillar from below the box down to the terrain so ramps/houses never float.
	struct PrefabPlacement
	{
		std::string prefabId;
		spk::Vector3Int anchor{};
		spk::VoxelOrientation orientation = spk::VoxelOrientation::PositiveZ;
		bool foundation = false;
		// Default placements center the rotated footprint on anchor.x/z and place its
		// lowest authored layer at anchor.y. Pivot-anchored placements instead land the
		// prefab's authored pivot exactly on anchor; multi-level stairs use this to pin
		// their top exit to the high plateau regardless of footprint size or rotation.
		bool anchorToPivot = false;
		TownBuildingRole townRole = TownBuildingRole::None;
	};

	// Which prefabs the generator inserts where, loaded from
	// resources/data/worldgen/placements.json (see parsePlanPlacementRules). Every slot
	// is a pool: one entry is picked at random per placement, so several variants add
	// diversity. Entity kinds without an entry simply get no prefab.
	struct PlanPlacementRules
	{
		std::map<PlanEntityKind, std::vector<std::string>> entityPrefabs;
	};

	// Axis-aligned world-column rectangle claimed by one committed stairway piece (a
	// flight, a platform, or the walkway lane beside a composed climb). The preview
	// map paints these in road color so the drawn road network shows where a climb
	// actually detours the path.
	struct PlanStairRect
	{
		int minX = 0;
		int maxX = 0;
		int minZ = 0;
		int maxZ = 0;
	};

	struct PlanClaim
	{
		spk::Vector3Int min{};
		spk::Vector3Int max{};
		friend bool operator==(const PlanClaim &, const PlanClaim &) = default;
	};

	// How a committed staircase crosses its cliff. Taller climbs try the layouts in
	// this order; one-level climbs are always a single perpendicular ramp.
	enum class StairLayout : std::uint8_t
	{
		OnePass,	   // one straight run descending along the cliff wall
		Switchback,	   // two opposing parallel runs joined by a landing
		Perpendicular  // straight flight crossing away from the cliff (fallback, and every one-level ramp)
	};

	// One committed staircase (road or wild, one-level ramps included), with its
	// exact top-to-bottom walking route. It is recorded at commit time so consumers
	// (the --check-stairs harness, the preview map, the realization, future gameplay)
	// never re-infer the group from the flat placement list. Anchor y's are stand
	// heights: topAnchor.y on the high plateau, bottomAnchor.y on the low ground.
	struct PlanStairway
	{
		StairLayout layout = StairLayout::Perpendicular;
		spk::Vector3Int topAnchor{};	// top stand column, flush with the high plateau
		spk::Vector3Int bottomAnchor{}; // bottom stand column on the low ground
		spk::Vector3Int plateauCell{};  // high-ground cell immediately beyond the top exit
		std::vector<spk::Vector3Int> centerPath; // ordered walk columns from top to bottom
		std::size_t firstPlacement = 0; // start of this staircase's contiguous run in WorldPlan::placements
		std::size_t placementCount = 0; // prefab placements stamped for this staircase
		// Every rectangle this staircase validated and reserved: the stamped pieces
		// plus the checked walkway lane and the plateau exit. The preview map paints
		// them in road color.
		std::vector<PlanStairRect> footprints;
		// Road climbs only: the flat approach lane connecting the road dead-end to the
		// bottom platform; realization paves these columns with the zone's road block.
		std::optional<PlanStairRect> pavedApproach;
		int lowRow = 0; // plan cell of the low ground at the crossing
		int lowCol = 0;
		int steps = 0;	   // height levels climbed == flight piece count
		bool road = false; // road climb (paved approach band) vs wild slope
	};

	// A one-way teleport: an actor whose cell reaches `from` (the block it stands on)
	// is moved to stand on `to`. Door cells of buildings pair with the entry pad of
	// their composed interior; the interior's exit pad pairs back with the cell just
	// outside the door.
	struct PlanPortal
	{
		spk::Vector3Int from{};
		spk::Vector3Int to{};
	};

	// A town site is reserved before the global road graph is built.  It owns the
	// stable settlement center and an extent, not a building layout.
	struct PlanTownSite
	{
		std::size_t macroEntityIndex = 0;
		PlanEntityKind kind = PlanEntityKind::City;
		std::string compositionId;
		int centerRow = 0;
		int centerCol = 0;
		int radiusColumns = 0;
	};

	struct PlanPavedColumn
	{
		int worldX = 0;
		int worldZ = 0;
		int surfaceY = 0;
		friend bool operator==(const PlanPavedColumn &, const PlanPavedColumn &) = default;
		friend bool operator<(const PlanPavedColumn &p_left, const PlanPavedColumn &p_right)
		{
			return std::tie(p_left.worldZ, p_left.worldX, p_left.surfaceY) < std::tie(p_right.worldZ, p_right.worldX, p_right.surfaceY);
		}
	};

	struct ResolvedTownEntranceRecord
	{
		std::string buildingInstanceId;
		spk::Vector3Int threshold{};
		spk::Vector3Int outward{};
		std::vector<std::pair<int, int>> approachColumns;
		std::pair<int, int> connectionPoint{};
	};

	struct UrbanRouteRecord
	{
		std::string buildingInstanceId;
		int cost = 0;
		std::vector<std::pair<int, int>> centerline;
	};

	struct TownWorldBounds
	{
		int minX = 0;
		int maxX = -1;
		int minZ = 0;
		int maxZ = -1;
	};

	// Exact paved columns, resolved entrances, and placement ownership are the
	// authority for a committed town.  No consumer should infer these from the
	// macro townPath compatibility cache.
	struct PlanTownRecord
	{
		PlanEntityKind kind = PlanEntityKind::City;
		std::size_t macroEntityIndex = 0;
		std::string compositionId;
		TownWorldBounds bounds{};
		int centerRow = 0;
		int centerCol = 0;
		std::vector<std::pair<int, int>> boundaryCells;
		std::vector<PlanPavedColumn> mainRoadSurface;
		std::vector<PlanPavedColumn> urbanRoadSurface;
		std::vector<ResolvedTownEntranceRecord> entrances;
		std::vector<UrbanRouteRecord> routes;
		std::vector<std::pair<int, int>> dockCells;
		std::vector<std::size_t> buildingPlacementIndices;
		std::vector<std::size_t> roadSceneryPlacementIndices;
		std::vector<std::size_t> groundSceneryPlacementIndices;
		std::optional<spk::Vector3Int> boardingEndpoint;
		std::vector<std::size_t> boatLinkIndices;
	};

	struct WorldGenConfig
	{
		static constexpr int MinimumPlanSize = 16;
		static constexpr int MaximumPlanSize = 512;
		static constexpr int MaximumZoneCount = 256;
		static constexpr int MaximumHeightLevel = std::numeric_limits<std::int8_t>::max();
		static constexpr int MaximumPerZoneCount = 64;
		static constexpr int MaximumBlocksPerCell = 256;
		static constexpr int MaximumBlocksPerLevel = 64;
		static constexpr int MaximumGroundMagnitude = 1'000'000;
		static constexpr int MaximumInteriorGap = 1'000'000;
		static constexpr int MinimumInteriorSlotBlocks = 8;
		static constexpr int MaximumInteriorSlotBlocks = 4096;

		int size = 248; // [MinimumPlanSize, MaximumPlanSize] plan cells per side
		int zoneCount = 8; // [1, min(MaximumZoneCount, size * size)]
		std::uint64_t masterSeed = 1234; // the complete uint64_t domain is valid

		// Territory / landmass
		double landThreshold = 0.42; // finite, strictly between 0 and 1
		double coastNoise = 0.55; // finite [0, 2]
		double fragmentation = 0.0; // finite [0, 1]
		double minZoneFraction = 0.03; // finite (0, 1]

		// Heights (in strata levels)
		int maxHeightLevel = 6; // [1, MaximumHeightLevel], fits PlanGrid<int8_t>
		double cellsPerLevel = 10.0; // finite (0, 4096]
		double coastTrendWeight = 0.5; // finite [0, 1]
		double undulationLevels = 2.2; // finite [0, MaximumHeightLevel]
		double heightFeatureCells = 26.0; // finite (0, 4096]

		// Rivers / lakes
		bool enableRivers = true;
		double riversPerZone = 0.5; // finite [0, MaximumPerZoneCount]
		double lakeMinDepth = 0.9; // finite (0, MaximumHeightLevel]
		int lakeMinSize = 6; // [1, lakeMaxSize]
		int lakeMaxSize = 40; // [lakeMinSize, size * size]

		// POI quotas (per zone), each in [0, MaximumPerZoneCount]. Settlement
		// counts are instead derived from each biome's townDistanceCells.
		int normalPoiPerZone = 5;
		int uncommonPoiPerZone = 2;
		int rarePoiPerZone = 2;
		int poiRoadConnections = 3;

		// Blocking radii (plan cells), each finite in [0, size].
		double blockGym = 5.0;
		double blockCity = 3.0;
		double blockRare = 4.0;
		double blockUncommon = 3.5;
		double blockNormal = 3.0;

		// POI spread spacing as a ratio of the zone radius, finite in [0, 1].
		double distRatioPoi = 0.35;

		// Coast rules
		double coastDistCells = 4.0; // finite [0, size]

		// Wild staircases: off-road ramps across strata cliffs, per zone, so the player
		// can climb levels away from the road network.
		int wildStairsPerZone = 4; // [0, MaximumPerZoneCount]
		double wildStairSpacingCells = 4.0; // finite [0, size]
		int maxRoadStairLevels = 6; // [1, maxComposedStairLevels], perpendicular fallback cap
		int maxWildStairLevels = 3; // [1, maxComposedStairLevels]
		int maxComposedStairLevels = 6; // [1, maxHeightLevel]

		// Maximum deterministic relocation distance for a town composition site.
		int townSearchRadiusCells = 18;
		// Whole-world retries after a typed atomic town-planning failure. Attempt zero
		// always uses masterSeed; later attempts use stable derived seeds.
		int maxTownWorldRetries = 8;

		// Realization (voxel) parameters
		int blocksPerCell = 8; // [4, MaximumBlocksPerCell]
		int blocksPerLevel = 3; // [1, min(MaximumBlocksPerLevel, blocksPerCell)]
		int groundLevelTop = 3; // [-MaximumGroundMagnitude, MaximumGroundMagnitude]

		// Interiors: composed room layouts live in a reserved band of columns past the
		// east edge of the world (terrain realization leaves the band void), one square
		// slot per interior, entered through door portals.
		int interiorRegionGap = 128; // [0, MaximumInteriorGap]
		int interiorSlotBlocks = 64; // [MinimumInteriorSlotBlocks, MaximumInteriorSlotBlocks]
	};

	struct WorldPlanStats
	{
		int continents = 0;
		int landCells = 0;
		int roadCells = 0;
		int roadComponents = 0;
		int roadSquares = 0; // 2x2 blocks of road cells (MUST be 0: roads stay one cell wide)
		int boatLinks = 0;
		int gymOnCoast = 0;
		int roadDiagonalSteps = 0;
		int riverDiagonalSteps = 0;
		int primaryGateways = 0;
		int secondaryGateways = 0;
		int riverCells = 0;
		int stairPlacements = 0;
		int wildStairCandidates = 0;
		int wildStairRatioSkips = 0;
		int wildStairSpacingSkips = 0;
		int wildStairPlacementRejects = 0;
		int wildStairPlacements = 0;
		int composedStairPlacements = 0;
		int rejectedStairways = 0;
		int sceneryPlacements = 0;
		int interiorCount = 0;			 // buildings that received a composed interior
		int interiorRoomPlacements = 0;	 // room prefabs stamped in the interior band
		int placementConflicts = 0;		 // claimed-zone collisions resolved by nudging or overriding
		int skippedPoiPlacements = 0;	 // POI prefabs dropped because no clear zone was found
		int townLayoutAttempts = 0;
		int townBuildingCandidates = 0;
		int townRouteExpansions = 0;
		std::vector<std::string> warnings;
	};

	// The full once-per-seed world skeleton: everything the voxel realization and the
	// preview image need. Immutable after generation, safe to share across threads.
	struct WorldPlan
	{
		WorldGenConfig config;
		std::vector<PlanBiome> biomes;

		PlanGrid<std::uint8_t> land;  // 1 = land
		PlanGrid<std::int16_t> zone;  // -1 = ocean
		PlanGrid<std::int8_t> height; // strata level, -1 = ocean
		PlanGrid<std::uint8_t> water; // river or lake
		PlanGrid<std::uint8_t> lake;  // kept lake cells
		PlanGrid<std::uint8_t> road;
		// Compatibility preview cache derived from exact town pavement.  It is not
		// used for town ownership, routing, or voxel realization.
		PlanGrid<std::uint8_t> townPath;
		PlanGrid<std::uint8_t> bridge; // road over water

		std::vector<PlanZone> zones;
		std::vector<PlanEntity> entities;
		std::vector<PlanTownSite> townSites;
		std::vector<PlanGateway> gateways;
		std::vector<std::pair<PlanEntity, PlanEntity>> boatLinks;
		std::vector<PrefabPlacement> placements;
		std::vector<PlanStairway> stairways; // every committed staircase, road and wild
		std::vector<PlanPortal> portals;
		std::vector<PlanTownRecord> towns;
		std::vector<PlanClaim> townClaims;

		WorldPlanStats stats;

		// World-cell x/z of the minimum corner of plan cell (0,0); the world is centered
		// on the voxel origin.
		[[nodiscard]] int worldOffset() const
		{
			const std::int64_t width = static_cast<std::int64_t>(config.size) * config.blocksPerCell;
			const std::int64_t result = -width / 2;
			if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			{
				throw std::overflow_error("WorldPlan world offset is outside the int coordinate range");
			}
			return static_cast<int>(result);
		}
		// Plan-cell index of a world column (same formula on both axes).
		[[nodiscard]] int cellIndexFromWorld(int p_worldCoordinate) const
		{
			if (config.blocksPerCell <= 0)
			{
				throw std::logic_error("WorldPlan blocksPerCell must be positive");
			}
			const std::int64_t shifted = static_cast<std::int64_t>(p_worldCoordinate) - worldOffset();
			const std::int64_t result = shifted >= 0
										? shifted / config.blocksPerCell
										: (shifted - config.blocksPerCell + 1) / config.blocksPerCell;
			if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			{
				throw std::overflow_error("WorldPlan cell index is outside the int coordinate range");
			}
			return static_cast<int>(result);
		}
		// World y of the surface block of a cell sitting at p_level.
		[[nodiscard]] int surfaceY(int p_level) const
		{
			const std::int64_t result = static_cast<std::int64_t>(config.groundLevelTop) +
									static_cast<std::int64_t>(p_level) * config.blocksPerLevel;
			if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			{
				throw std::overflow_error("WorldPlan surface height is outside the int coordinate range");
			}
			return static_cast<int>(result);
		}
		// First column of the interior band; columns at or past it get no terrain.
		[[nodiscard]] int interiorRegionMinX() const
		{
			const std::int64_t result = -static_cast<std::int64_t>(worldOffset()) + config.interiorRegionGap;
			if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			{
				throw std::overflow_error("WorldPlan interior region is outside the int coordinate range");
			}
			return static_cast<int>(result);
		}
		[[nodiscard]] bool isInteriorColumn(int p_worldX) const
		{
			return p_worldX >= interiorRegionMinX();
		}

		[[nodiscard]] std::string report() const;
	};

	struct BiomeDefinition;
	struct PrefabDefinition;
	struct InteriorDefinition;

	// Collects the worldgen-enabled biomes out of the loaded biome registry (sorted by
	// id, so generation stays deterministic for a given data set).
	[[nodiscard]] std::vector<PlanBiome> planBiomesFrom(const Registry<BiomeDefinition> &p_biomes);

	// Validates every field and derived allocation/coordinate bound. Throws
	// std::invalid_argument with the offending field name before generation allocates.
	void validateWorldGenConfig(const WorldGenConfig &p_config);
	void validateWorldPlanTowns(const WorldPlan &p_plan);

	[[nodiscard]] WorldPlan generateWorldPlan(
		const WorldGenConfig &p_config,
		const std::vector<PlanBiome> &p_biomes,
		const PlanPlacementRules &p_placementRules,
		const Registry<PrefabDefinition> &p_prefabs,
		const TownCompositionCatalog &p_townCompositions,
		const Registry<InteriorDefinition> &p_interiors);
}
