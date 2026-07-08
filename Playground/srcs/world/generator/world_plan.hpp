#pragma once

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pg
{
	// Square per-plan-cell grid. A plan cell is the macro unit of the world skeleton
	// (one cell = blocksPerCell x blocksPerCell voxel columns once realized).
	template <typename TValue>
	class PlanGrid
	{
	private:
		int _size = 0;
		std::vector<TValue> _values;

	public:
		PlanGrid() = default;
		explicit PlanGrid(int p_size, TValue p_fill = TValue{}) :
			_size(p_size),
			_values(static_cast<std::size_t>(p_size) * static_cast<std::size_t>(p_size), p_fill)
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
			return _values[static_cast<std::size_t>(p_row) * _size + p_col];
		}
		[[nodiscard]] const TValue &at(int p_row, int p_col) const
		{
			return _values[static_cast<std::size_t>(p_row) * _size + p_col];
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
		// placement. Stairways are not configured here: they resolve by convention from the
		// biome id ("<id>-stair-length" for the flights, "<id>-stair-platform" for the pads).
		std::map<PlanEntityKind, std::vector<std::string>> entityPrefabs;
		// Decorative structures scattered on clear land in this biome. Unlike POIs these
		// have no gameplay role and may be multi-voxel prefabs such as trees or plants.
		std::vector<PlanScenery> scenery;
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

	// A one-way teleport: an actor whose cell reaches `from` (the block it stands on)
	// is moved to stand on `to`. Door cells of buildings pair with the entry pad of
	// their composed interior; the interior's exit pad pairs back with the cell just
	// outside the door.
	struct PlanPortal
	{
		spk::Vector3Int from{};
		spk::Vector3Int to{};
	};

	struct WorldGenConfig
	{
		int size = 124; // plan cells per side
		int zoneCount = 8;
		std::uint64_t masterSeed = 1234;

		// Territory / landmass
		double landThreshold = 0.42;
		double coastNoise = 0.55;
		double fragmentation = 0.0;
		double minZoneFraction = 0.03;

		// Heights (in strata levels)
		int maxHeightLevel = 6;
		double cellsPerLevel = 10.0;
		double coastTrendWeight = 0.5;
		double undulationLevels = 2.2;
		double heightFeatureCells = 26.0;

		// Rivers / lakes
		bool enableRivers = true;
		double riversPerZone = 0.5;
		double lakeMinDepth = 0.9;
		int lakeMinSize = 6;
		int lakeMaxSize = 40;

		// Settlement quotas (per zone)
		int gymsPerZone = 1;
		int citiesPerZone = 3;
		int normalPoiPerZone = 5;
		int uncommonPoiPerZone = 2;
		int rarePoiPerZone = 2;
		int poiRoadConnections = 3;

		// Blocking radii (plan cells)
		double blockGym = 5.0;
		double blockCity = 3.0;
		double blockRare = 4.0;
		double blockUncommon = 3.5;
		double blockNormal = 3.0;

		// Spread spacing as a ratio of the zone radius
		double distRatioCity = 0.55;
		double distRatioGym = 0.9;
		double distRatioPoi = 0.35;

		// Coast rules
		double coastDistCells = 4.0;
		int portsPerContinent = 1;

		// Wild staircases: off-road ramps across strata cliffs, per zone, so the player
		// can climb levels away from the road network.
		int wildStairsPerZone = 4;
		double wildStairSpacingCells = 4.0; // min distance between stairways
		int maxRoadStairLevels = 2;		// straight perpendicular runs that fit inside one plan cell
		int maxWildStairLevels = 3;		// tallest composed off-road staircase
		int maxComposedStairLevels = 6; // tallest composed road staircase (platform + N flights + platform)

		// Realization (voxel) parameters
		int blocksPerCell = 8;	// horizontal blocks per plan cell
		int blocksPerLevel = 3; // the requested 3-block strata
		int groundLevelTop = 3; // world y of the level-0 surface block

		// Interiors: composed room layouts live in a reserved band of columns past the
		// east edge of the world (terrain realization leaves the band void), one square
		// slot per interior, entered through door portals.
		int interiorRegionGap = 128; // void blocks between the world edge and the band
		int interiorSlotBlocks = 64; // square slot side per composed interior
	};

	struct WorldPlanStats
	{
		int continents = 0;
		int landCells = 0;
		int roadCells = 0;
		int roadComponents = 0;
		int boatLinks = 0;
		int gymOnCoast = 0;
		int roadDiagonalSteps = 0;
		int riverDiagonalSteps = 0;
		int primaryGateways = 0;
		int secondaryGateways = 0;
		int riverCells = 0;
		int stairPlacements = 0;
		int wildStairPlacements = 0;
		int composedStairPlacements = 0;
		int rejectedStairways = 0;
		int sceneryPlacements = 0;
		int interiorCount = 0;			 // buildings that received a composed interior
		int interiorRoomPlacements = 0;	 // room prefabs stamped in the interior band
		int placementConflicts = 0;		 // claimed-zone collisions resolved by nudging or overriding
		int skippedPoiPlacements = 0;	 // POI prefabs dropped because no clear zone was found
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
		PlanGrid<std::uint8_t> bridge; // road over water

		std::vector<PlanZone> zones;
		std::vector<PlanEntity> entities;
		std::vector<PlanGateway> gateways;
		std::vector<std::pair<PlanEntity, PlanEntity>> boatLinks;
		std::vector<PrefabPlacement> placements;
		std::vector<std::pair<int, int>> wildStairs; // (row, col) of each wild stairway's lower cell
		std::vector<PlanStairRect> stairRects;		 // world-column footprints of every stairway
		std::vector<PlanPortal> portals;

		WorldPlanStats stats;

		// World-cell x/z of the minimum corner of plan cell (0,0); the world is centered
		// on the voxel origin.
		[[nodiscard]] int worldOffset() const noexcept
		{
			return -(config.size * config.blocksPerCell) / 2;
		}
		// Plan-cell index of a world column (same formula on both axes).
		[[nodiscard]] int cellIndexFromWorld(int p_worldCoordinate) const noexcept
		{
			const int shifted = p_worldCoordinate - worldOffset();
			return shifted >= 0 ? shifted / config.blocksPerCell
								: (shifted - config.blocksPerCell + 1) / config.blocksPerCell;
		}
		// World y of the surface block of a cell sitting at p_level.
		[[nodiscard]] int surfaceY(int p_level) const noexcept
		{
			return config.groundLevelTop + p_level * config.blocksPerLevel;
		}
		// First column of the interior band; columns at or past it get no terrain.
		[[nodiscard]] int interiorRegionMinX() const noexcept
		{
			return -worldOffset() + config.interiorRegionGap;
		}
		[[nodiscard]] bool isInteriorColumn(int p_worldX) const noexcept
		{
			return p_worldX >= interiorRegionMinX();
		}

		[[nodiscard]] std::string report() const;
	};

	template <typename TDefinition>
	class Registry;
	struct BiomeDefinition;
	struct PrefabDefinition;
	struct InteriorDefinition;

	// Collects the worldgen-enabled biomes out of the loaded biome registry (sorted by
	// id, so generation stays deterministic for a given data set).
	[[nodiscard]] std::vector<PlanBiome> planBiomesFrom(const Registry<BiomeDefinition> &p_biomes);

	[[nodiscard]] WorldPlan generateWorldPlan(
		const WorldGenConfig &p_config,
		const std::vector<PlanBiome> &p_biomes,
		const PlanPlacementRules &p_placementRules,
		const Registry<PrefabDefinition> &p_prefabs,
		const Registry<InteriorDefinition> &p_interiors);
}
