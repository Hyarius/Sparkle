#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/registries.hpp"
#include "world/generator/plan_chunk_provider.hpp"
#include "world/generator/path_surface.hpp"
#include "world/generator/town_blueprint.hpp"
#include "world/generator/town_planner.hpp"
#include "world/generator/stair_planner.hpp"
#include "world/generator/town_commit.hpp"
#include "world/generator/world_plan.hpp"
#include "world/prefab_placement_math.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

namespace
{
	using Config = pg::WorldGenConfig;

	const pg::Registries &loadedRegistries()
	{
		static pg::Registries registries;
		static const bool loaded = [] {
			registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return true;
		}();
		(void)loaded;
		return registries;
	}

	void expectInvalidField(const Config &p_config, const std::string &p_field)
	{
		try
		{
			pg::validateWorldGenConfig(p_config);
			FAIL() << "Expected invalid WorldGenConfig." << p_field;
		}
		catch (const std::invalid_argument &exception)
		{
			EXPECT_NE(std::string(exception.what()).find(p_field), std::string::npos)
				<< exception.what();
		}
	}
}

TEST(WorldPlanValidation, DefaultConfigurationAndFullSeedDomainAreValid)
{
	Config config;
	EXPECT_NO_THROW(pg::validateWorldGenConfig(config));
	config.masterSeed = 0;
	EXPECT_NO_THROW(pg::validateWorldGenConfig(config));
	config.masterSeed = std::numeric_limits<std::uint64_t>::max();
	EXPECT_NO_THROW(pg::validateWorldGenConfig(config));
}

TEST(PlanChunkProvider, StairFoundationUsesTerrainVoxels)
{
	const pg::Registries &registries = loadedRegistries();
	pg::WorldPlan plan;
	plan.config.size = 1;
	plan.config.blocksPerCell = 8;
	plan.config.blocksPerLevel = 3;
	plan.config.groundLevelTop = 3;
	plan.land = pg::PlanGrid<std::uint8_t>(1, 1);
	plan.zone = pg::PlanGrid<std::int16_t>(1, -1);
	plan.height = pg::PlanGrid<std::int8_t>(1, 0);
	plan.water = pg::PlanGrid<std::uint8_t>(1, 0);
	plan.lake = pg::PlanGrid<std::uint8_t>(1, 0);
	plan.road = pg::PlanGrid<std::uint8_t>(1, 0);
	plan.bridge = pg::PlanGrid<std::uint8_t>(1, 0);
	const int stairY = spk::VoxelChunk::Size.y + 10;
	plan.placements.push_back(
		{.prefabId = "stair-length", .anchor = {0, stairY, 0}, .foundation = true});

	const pg::PlanChunkProvider provider(registries, plan);
	const spk::Vector3Int foundationPosition{0, spk::VoxelChunk::Size.y - 1, -1};
	spk::VoxelChunk foundationChunk(
		spk::VoxelChunk::coordinatesFromWorldCell(foundationPosition),
		registries.voxels().renderRegistry());
	provider.fill(foundationChunk);
	const auto foundationCellAt = [&](int p_y) -> const spk::VoxelCell & {
		return foundationChunk.cell(
			foundationChunk.localFromWorld({foundationPosition.x, p_y, foundationPosition.z}));
	};

	const spk::VoxelRuntimeId terrainId = registries.voxels().runtimeId("stone-block");
	for (int y = plan.config.groundLevelTop + 1; y < spk::VoxelChunk::Size.y; ++y)
	{
		EXPECT_EQ(foundationCellAt(y).id, terrainId) << "foundation voxel at y=" << y;
	}

	const spk::Vector3Int stairPosition{foundationPosition.x, stairY, foundationPosition.z};
	spk::VoxelChunk stairChunk(
		spk::VoxelChunk::coordinatesFromWorldCell(stairPosition),
		registries.voxels().renderRegistry());
	provider.fill(stairChunk);
	EXPECT_EQ(
		stairChunk.cell(stairChunk.localFromWorld(stairPosition)).id,
		registries.voxels().numericId("road-stair"));
}

TEST(WorldPlanValidation, RejectsEveryIntegerFieldOutsideItsDocumentedRange)
{
	struct InvalidInteger
	{
		const char *field;
		std::function<void(Config &)> mutate;
	};
	const std::vector<InvalidInteger> cases = {
		{"size", [](Config &p) { p.size = 0; }},
		{"size", [](Config &p) { p.size = Config::MaximumPlanSize + 1; }},
		{"zoneCount", [](Config &p) { p.zoneCount = 0; }},
		{"zoneCount", [](Config &p) { p.zoneCount = Config::MaximumZoneCount + 1; }},
		{"maxHeightLevel", [](Config &p) { p.maxHeightLevel = 0; }},
		{"maxHeightLevel", [](Config &p) { p.maxHeightLevel = Config::MaximumHeightLevel + 1; }},
		{"lakeMinSize", [](Config &p) { p.lakeMinSize = 0; }},
		{"lakeMinSize", [](Config &p) { p.lakeMinSize = Config::MaximumPlanSize * Config::MaximumPlanSize + 1; }},
		{"lakeMaxSize", [](Config &p) { p.lakeMaxSize = 0; }},
		{"lakeMaxSize", [](Config &p) { p.lakeMaxSize = Config::MaximumPlanSize * Config::MaximumPlanSize + 1; }},
		{"gymsPerZone", [](Config &p) { p.gymsPerZone = -1; }},
		{"gymsPerZone", [](Config &p) { p.gymsPerZone = Config::MaximumPerZoneCount + 1; }},
		{"citiesPerZone", [](Config &p) { p.citiesPerZone = -1; }},
		{"citiesPerZone", [](Config &p) { p.citiesPerZone = Config::MaximumPerZoneCount + 1; }},
		{"normalPoiPerZone", [](Config &p) { p.normalPoiPerZone = -1; }},
		{"normalPoiPerZone", [](Config &p) { p.normalPoiPerZone = Config::MaximumPerZoneCount + 1; }},
		{"uncommonPoiPerZone", [](Config &p) { p.uncommonPoiPerZone = -1; }},
		{"uncommonPoiPerZone", [](Config &p) { p.uncommonPoiPerZone = Config::MaximumPerZoneCount + 1; }},
		{"rarePoiPerZone", [](Config &p) { p.rarePoiPerZone = -1; }},
		{"rarePoiPerZone", [](Config &p) { p.rarePoiPerZone = Config::MaximumPerZoneCount + 1; }},
		{"poiRoadConnections", [](Config &p) { p.poiRoadConnections = -1; }},
		{"poiRoadConnections", [](Config &p) { p.poiRoadConnections = Config::MaximumPerZoneCount + 1; }},
		{"portsPerContinent", [](Config &p) { p.portsPerContinent = -1; }},
		{"portsPerContinent", [](Config &p) { p.portsPerContinent = Config::MaximumPerZoneCount + 1; }},
		{"wildStairsPerZone", [](Config &p) { p.wildStairsPerZone = -1; }},
		{"wildStairsPerZone", [](Config &p) { p.wildStairsPerZone = Config::MaximumPerZoneCount + 1; }},
		{"maxRoadStairLevels", [](Config &p) { p.maxRoadStairLevels = 0; }},
		{"maxRoadStairLevels", [](Config &p) { p.maxRoadStairLevels = p.maxComposedStairLevels + 1; }},
		{"maxWildStairLevels", [](Config &p) { p.maxWildStairLevels = 0; }},
		{"maxWildStairLevels", [](Config &p) { p.maxWildStairLevels = p.maxComposedStairLevels + 1; }},
		{"maxComposedStairLevels", [](Config &p) { p.maxComposedStairLevels = 0; }},
		{"maxComposedStairLevels", [](Config &p) { p.maxComposedStairLevels = p.maxHeightLevel + 1; }},
		{"townSearchRadiusCells", [](Config &p) { p.townSearchRadiusCells = -1; }},
		{"townSearchRadiusCells", [](Config &p) { p.townSearchRadiusCells = Config::MaximumPlanSize + 1; }},
		{"maxTownWorldRetries", [](Config &p) { p.maxTownWorldRetries = -1; }},
		{"maxTownWorldRetries", [](Config &p) { p.maxTownWorldRetries = 65; }},
		{"blocksPerCell", [](Config &p) { p.blocksPerCell = 3; }},
		{"blocksPerCell", [](Config &p) { p.blocksPerCell = Config::MaximumBlocksPerCell + 1; }},
		{"blocksPerLevel", [](Config &p) { p.blocksPerLevel = 0; }},
		{"blocksPerLevel", [](Config &p) { p.blocksPerLevel = Config::MaximumBlocksPerLevel + 1; }},
		{"groundLevelTop", [](Config &p) { p.groundLevelTop = -Config::MaximumGroundMagnitude - 1; }},
		{"groundLevelTop", [](Config &p) { p.groundLevelTop = Config::MaximumGroundMagnitude + 1; }},
		{"interiorRegionGap", [](Config &p) { p.interiorRegionGap = -1; }},
		{"interiorRegionGap", [](Config &p) { p.interiorRegionGap = Config::MaximumInteriorGap + 1; }},
		{"interiorSlotBlocks", [](Config &p) { p.interiorSlotBlocks = Config::MinimumInteriorSlotBlocks - 1; }},
		{"interiorSlotBlocks", [](Config &p) { p.interiorSlotBlocks = Config::MaximumInteriorSlotBlocks + 1; }}};

	for (const InvalidInteger &entry : cases)
	{
		Config config;
		entry.mutate(config);
		expectInvalidField(config, entry.field);
	}
}

TEST(WorldPlanValidation, RejectsNonFiniteAndOutOfRangeFloatingFields)
{
	struct FloatingField
	{
		const char *name;
		double Config::*member;
		double outside;
	};
	const std::array fields = {
		FloatingField{"landThreshold", &Config::landThreshold, 0.0},
		FloatingField{"coastNoise", &Config::coastNoise, -0.1},
		FloatingField{"fragmentation", &Config::fragmentation, 1.1},
		FloatingField{"minZoneFraction", &Config::minZoneFraction, 0.0},
		FloatingField{"cellsPerLevel", &Config::cellsPerLevel, 0.0},
		FloatingField{"coastTrendWeight", &Config::coastTrendWeight, 1.1},
		FloatingField{"undulationLevels", &Config::undulationLevels, -0.1},
		FloatingField{"heightFeatureCells", &Config::heightFeatureCells, 0.0},
		FloatingField{"riversPerZone", &Config::riversPerZone, -0.1},
		FloatingField{"lakeMinDepth", &Config::lakeMinDepth, 0.0},
		FloatingField{"blockGym", &Config::blockGym, -0.1},
		FloatingField{"blockCity", &Config::blockCity, -0.1},
		FloatingField{"blockRare", &Config::blockRare, -0.1},
		FloatingField{"blockUncommon", &Config::blockUncommon, -0.1},
		FloatingField{"blockNormal", &Config::blockNormal, -0.1},
		FloatingField{"distRatioCity", &Config::distRatioCity, 1.1},
		FloatingField{"distRatioGym", &Config::distRatioGym, 1.1},
		FloatingField{"distRatioPoi", &Config::distRatioPoi, 1.1},
		FloatingField{"coastDistCells", &Config::coastDistCells, -0.1},
		FloatingField{"wildStairSpacingCells", &Config::wildStairSpacingCells, -0.1}};

	for (const FloatingField &field : fields)
	{
		for (const double invalid : {
				 field.outside,
				 std::numeric_limits<double>::quiet_NaN(),
				 std::numeric_limits<double>::infinity()})
		{
			Config config;
			config.*(field.member) = invalid;
			expectInvalidField(config, field.name);
		}
	}
}

TEST(WorldPlanValidation, RejectsInvertedLakeAndStairLimits)
{
	Config config;
	config.lakeMinSize = config.lakeMaxSize + 1;
	expectInvalidField(config, "lakeMinSize");

	config = Config{};
	config.maxRoadStairLevels = config.maxComposedStairLevels + 1;
	expectInvalidField(config, "maxRoadStairLevels");

	config = Config{};
	config.maxWildStairLevels = config.maxComposedStairLevels + 1;
	expectInvalidField(config, "maxWildStairLevels");
}

TEST(WorldPlanValidation, ValidatesCompactStorageAndDerivedProductBoundaries)
{
	Config maximum;
	maximum.size = Config::MaximumPlanSize;
	maximum.zoneCount = Config::MaximumZoneCount;
	maximum.maxHeightLevel = Config::MaximumHeightLevel;
	maximum.lakeMaxSize = maximum.size * maximum.size;
	maximum.maxComposedStairLevels = maximum.maxHeightLevel;
	maximum.maxRoadStairLevels = maximum.maxComposedStairLevels;
	maximum.maxWildStairLevels = maximum.maxComposedStairLevels;
	maximum.blocksPerCell = Config::MaximumBlocksPerCell;
	maximum.blocksPerLevel = Config::MaximumBlocksPerLevel;
	maximum.groundLevelTop = Config::MaximumGroundMagnitude;
	maximum.interiorRegionGap = Config::MaximumInteriorGap;
	maximum.interiorSlotBlocks = Config::MaximumInteriorSlotBlocks;
	EXPECT_NO_THROW(pg::validateWorldGenConfig(maximum));

	Config zoneOverflow;
	zoneOverflow.zoneCount = static_cast<int>(std::numeric_limits<std::int16_t>::max()) + 2;
	expectInvalidField(zoneOverflow, "zoneCount");
	Config heightOverflow;
	heightOverflow.maxHeightLevel = static_cast<int>(std::numeric_limits<std::int8_t>::max()) + 1;
	expectInvalidField(heightOverflow, "maxHeightLevel");
	Config areaOverflow;
	areaOverflow.size = Config::MaximumPlanSize + 1;
	expectInvalidField(areaOverflow, "size");
	Config extentOverflow;
	extentOverflow.blocksPerCell = Config::MaximumBlocksPerCell + 1;
	expectInvalidField(extentOverflow, "blocksPerCell");
}

TEST(PlanGridValidation, RejectsInvalidDimensionsAndCoordinates)
{
	EXPECT_THROW((pg::PlanGrid<int>(0)), std::invalid_argument);
	EXPECT_THROW((pg::PlanGrid<int>(-1)), std::invalid_argument);
	EXPECT_THROW((pg::PlanGrid<int>(std::numeric_limits<int>::max())), std::length_error);

	pg::PlanGrid<int> grid(2, 7);
	EXPECT_EQ(grid.at(0, 0), 7);
	EXPECT_THROW((void)grid.at(-1, 0), std::out_of_range);
	EXPECT_THROW((void)grid.at(0, 2), std::out_of_range);
	const pg::PlanGrid<int> &constGrid = grid;
	EXPECT_THROW((void)constGrid.at(2, 0), std::out_of_range);
}

TEST(WorldPlanValidation, CoordinateHelpersUseCheckedWideArithmetic)
{
	pg::WorldPlan plan;
	plan.config = Config{};
	EXPECT_EQ(plan.worldOffset(), -(plan.config.size * plan.config.blocksPerCell) / 2);
	EXPECT_NO_THROW((void)plan.cellIndexFromWorld(std::numeric_limits<int>::min()));
	EXPECT_NO_THROW((void)plan.cellIndexFromWorld(std::numeric_limits<int>::max()));

	plan.config.size = std::numeric_limits<int>::max();
	plan.config.blocksPerCell = std::numeric_limits<int>::max();
	EXPECT_THROW((void)plan.worldOffset(), std::overflow_error);
	plan.config = Config{};
	plan.config.groundLevelTop = std::numeric_limits<int>::max();
	EXPECT_THROW((void)plan.surfaceY(1), std::overflow_error);
	plan.config.blocksPerCell = 0;
	EXPECT_THROW((void)plan.cellIndexFromWorld(0), std::logic_error);
}

TEST(WorldPlanValidation, DefaultGenerationIsDeterministic)
{
	const pg::Registries &registries = loadedRegistries();
	const Config config;
	const auto generate = [&] {
		return pg::generateWorldPlan(
			config,
			pg::planBiomesFrom(registries.biomes()),
			registries.placementRules(),
			registries.prefabs(),
			registries.townBlueprints(),
			registries.interiors());
	};

	const pg::WorldPlan first = generate();
	const pg::WorldPlan second = generate();
	EXPECT_EQ(first.report(), second.report());
	EXPECT_EQ(first.land.at(config.size / 2, config.size / 2),
		second.land.at(config.size / 2, config.size / 2));
	EXPECT_EQ(first.zone.at(config.size / 2, config.size / 2),
		second.zone.at(config.size / 2, config.size / 2));
}

TEST(WorldPlanValidation, TownFailureRetriesTheWholeWorldDeterministically)
{
	const pg::Registries &registries=loadedRegistries();Config config;config.masterSeed=13;const auto generate=[&]{return pg::generateWorldPlan(config,pg::planBiomesFrom(registries.biomes()),registries.placementRules(),registries.prefabs(),registries.townBlueprints(),registries.interiors());};const pg::WorldPlan first=generate(),second=generate();EXPECT_NE(first.config.masterSeed,config.masterSeed);EXPECT_EQ(first.config.masterSeed,second.config.masterSeed);EXPECT_EQ(first.report(),second.report());EXPECT_TRUE(std::ranges::any_of(first.stats.warnings,[](const std::string &warning){return warning.find("deterministic town-generation retry")!=std::string::npos;}));
}

TEST(WorldPlanValidation, EverySettlementUsesExactlyOneBlueprintWriter)
{
	const pg::Registries &registries = loadedRegistries();
	Config config;
	config.masterSeed = 42;
	const pg::WorldPlan plan = pg::generateWorldPlan(
		config,
		pg::planBiomesFrom(registries.biomes()),
		registries.placementRules(),
		registries.prefabs(),
		registries.townBlueprints(),
		registries.interiors());

	const int settlements = static_cast<int>(std::ranges::count_if(plan.entities, [](const pg::PlanEntity &p_entity) {
		return p_entity.kind == pg::PlanEntityKind::City || p_entity.kind == pg::PlanEntityKind::Gym ||
			p_entity.kind == pg::PlanEntityKind::PortCity;
	}));
	ASSERT_GT(settlements, 0);
	EXPECT_EQ(plan.stats.blueprintTownWriters, settlements);
	EXPECT_EQ(plan.towns.size(), static_cast<std::size_t>(settlements));
}

TEST(TownBlueprint, RotatesNonSquareCellsAndRectanglesInEveryQuarterTurn)
{
	const pg::LocalRect bounds{{0, 0}, {1, 2}};
	EXPECT_EQ(pg::rotateTownCell({0, 0}, bounds, 0), (pg::LocalCell{0, 0}));
	EXPECT_EQ(pg::rotateTownCell({0, 0}, bounds, 1), (pg::LocalCell{0, 1}));
	EXPECT_EQ(pg::rotateTownCell({0, 0}, bounds, 2), (pg::LocalCell{1, 2}));
	EXPECT_EQ(pg::rotateTownCell({0, 0}, bounds, 3), (pg::LocalCell{2, 0}));
	EXPECT_EQ(pg::rotateTownRect(bounds, bounds, 1).max, (pg::LocalCell{2, 1}));
	EXPECT_EQ(pg::rotateTownRect(bounds, bounds, 3).max, (pg::LocalCell{2, 1}));
}

TEST(TownBlueprint, CityPlazaFlatLoadsWithConnectedRequiredDoorApproaches)
{
	const pg::TownBlueprint &blueprint = loadedRegistries().townBlueprints().get("city-plaza-flat");
	EXPECT_EQ(blueprint.kind, pg::TownBlueprintKind::City);
	EXPECT_EQ(blueprint.allowedQuarterTurns, (std::vector<int>{0, 1, 2, 3}));
	EXPECT_EQ(blueprint.lots.size(), 5u);
	EXPECT_FALSE(pg::renderTownBlueprintAscii(blueprint, 1).empty());
}

TEST(TownPlanner, FlatCandidateResolutionIsPureAndDeterministic)
{
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1); plan.water = pg::PlanGrid<std::uint8_t>(16, 0); plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	const pg::TownBlueprint &blueprint = loadedRegistries().townBlueprints().get("city-plaza-flat");
	const pg::TownPlanResult first = pg::resolveFlatTownCandidate(plan, blueprint, 3, 3, 0);
	const pg::TownPlanResult second = pg::resolveFlatTownCandidate(plan, blueprint, 3, 3, 0);
	ASSERT_TRUE(first.candidate.has_value()); ASSERT_TRUE(second.candidate.has_value());
	EXPECT_EQ(first.candidate->pathCells, second.candidate->pathCells);
	EXPECT_EQ(plan.water.at(3, 3), 0);
}

TEST(TownPlanner, RequiresTheAuthoredEntranceToMeetTheGlobalRoad)
{
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1);
	plan.water = pg::PlanGrid<std::uint8_t>(16, 0);
	plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	plan.road = pg::PlanGrid<std::uint8_t>(16, 0);
	const auto result = pg::resolveFlatTownCandidate(
		plan, loadedRegistries().townBlueprints().get("city-plaza-flat"), 3, 3, 0);
	ASSERT_TRUE(result.candidate.has_value());
	EXPECT_EQ(result.candidate->entranceCell, (pg::LocalCell{3, 6}));
	EXPECT_FALSE(pg::hasValidGlobalRoadArrival(plan, *result.candidate));

	plan.road.at(3, 6) = 1;
	EXPECT_TRUE(pg::hasValidGlobalRoadArrival(plan, *result.candidate));
}

TEST(TownPlanner, WaterRejectionHasNoObservablePlanMutation)
{
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1); plan.water = pg::PlanGrid<std::uint8_t>(16, 0); plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	plan.water.at(3, 3) = 1;
	const pg::TownMutationSnapshot before = pg::snapshotTownMutation(plan);
	const auto result = pg::resolveFlatTownCandidate(plan, loadedRegistries().townBlueprints().get("city-plaza-flat"), 3, 3, 0);
	ASSERT_TRUE(result.rejection.has_value());
	EXPECT_EQ(result.rejection->category, pg::TownRejectCategory::Water);
	EXPECT_TRUE(pg::matchesTownMutationSnapshot(plan, before));
}

TEST(TownPlanner, ContentResolutionIsDeterministicAndReportsPrefabAndDoorFailures)
{
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1); plan.water = pg::PlanGrid<std::uint8_t>(16, 0); plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	pg::TownBlueprint blueprint = loadedRegistries().townBlueprints().get("city-plaza-flat");
	blueprint.lots = {blueprint.lots.front()}; // one lot isolates prefab resolution from inter-lot claims
	const std::vector<pg::PlanBiome> biomes = pg::planBiomesFrom(loadedRegistries().biomes());
	const pg::PlanBiome &biome = biomes.front();
	ASSERT_TRUE(biome.town.has_value());
	const pg::TownContentContext valid{.prefabs = loadedRegistries().prefabs(), .town = *biome.town, .worldSeed = 1234, .stableEntityId = "city:1"};
	const auto first = pg::resolveFlatTownCandidate(plan, blueprint, valid, 3, 3, 0);
	const auto second = pg::resolveFlatTownCandidate(plan, blueprint, valid, 3, 3, 0);
	ASSERT_TRUE(first.candidate.has_value()); ASSERT_TRUE(second.candidate.has_value());
	ASSERT_EQ(first.candidate->buildings.size(), 1u);
	EXPECT_EQ(first.candidate->buildings.front().prefabId, second.candidate->buildings.front().prefabId);
	EXPECT_EQ(first.candidate->buildings.front().orientation, spk::VoxelOrientation::NegativeX)
		<< "the center west of the street must face east toward its approach";

	pg::PlanTown missing = *biome.town; missing.creatureCenter = "does-not-exist";
	const auto missingResult = pg::resolveFlatTownCandidate(plan, blueprint, {.prefabs = loadedRegistries().prefabs(), .town = missing}, 3, 3, 0);
	ASSERT_TRUE(missingResult.rejection.has_value());
	EXPECT_EQ(missingResult.rejection->category, pg::TownRejectCategory::MissingPrefab);

	pg::PlanTown doorless = *biome.town; doorless.creatureCenter = "town-light-pole";
	const auto doorResult = pg::resolveFlatTownCandidate(plan, blueprint, {.prefabs = loadedRegistries().prefabs(), .town = doorless}, 3, 3, 0);
	ASSERT_TRUE(doorResult.rejection.has_value());
	EXPECT_EQ(doorResult.rejection->category, pg::TownRejectCategory::DoorMismatch);
}

TEST(TownPlanner, PortBuildingsFaceTheirIndividualStreetApproaches)
{
	const pg::Registries &registries = loadedRegistries();
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1);
	plan.water = pg::PlanGrid<std::uint8_t>(16, 0);
	plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	plan.land.at(3, 9) = 0;
	const std::vector<pg::PlanBiome> biomes = pg::planBiomesFrom(registries.biomes());
	const pg::PlanBiome &biome = biomes.front();
	ASSERT_TRUE(biome.town.has_value());
	const auto result = pg::resolveFlatTownCandidate(
		plan,
		registries.townBlueprints().get("port-plaza-flat"),
		{.prefabs = registries.prefabs(), .town = *biome.town, .worldSeed = 9, .stableEntityId = "port"},
		3,
		3,
		0);
	ASSERT_TRUE(result.candidate.has_value())
		<< "category=" << static_cast<int>(result.rejection->category)
		<< " component=" << result.rejection->componentId;
	ASSERT_EQ(result.candidate->buildings.size(), 5u);
	for (const pg::PrefabPlacement &building : result.candidate->buildings)
	{
		EXPECT_EQ(building.orientation, spk::VoxelOrientation::PositiveZ);
	}
}

TEST(TownPlanner, SlopeRejectionIsPure)
{
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1); plan.water = pg::PlanGrid<std::uint8_t>(16, 0); plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	plan.height.at(7, 6) = 3;
	const auto result = pg::resolveFlatTownCandidate(plan, loadedRegistries().townBlueprints().get("city-plaza-flat"), 3, 3, 0);
	ASSERT_TRUE(result.rejection.has_value());
	EXPECT_EQ(result.rejection->category, pg::TownRejectCategory::TerraceHeight);
}

TEST(TownPlanner, SiteEnumerationIsStableCenterOutRingOrder)
{
	const std::vector<pg::LocalCell> origins = pg::enumerateTownOrigins({10, 10}, 1);
	EXPECT_EQ(origins, (std::vector<pg::LocalCell>{{10,10}, {9,9}, {9,10}, {9,11}, {10,11}, {11,11}, {11,10}, {11,9}, {10,9}}));
	EXPECT_EQ(pg::enumerateTownOrigins({0, 0}, 2).size(), 25u);
}

TEST(TownPlanner, OutOfBoundsAndOverlappingLotsAreRejectedWithoutCommit)
{
	pg::WorldPlan plan;
	plan.config.size = 16;
	plan.land = pg::PlanGrid<std::uint8_t>(16, 1); plan.water = pg::PlanGrid<std::uint8_t>(16, 0); plan.height = pg::PlanGrid<std::int8_t>(16, 2);
	const pg::TownBlueprint &source = loadedRegistries().townBlueprints().get("city-plaza-flat");
	const auto outOfBounds = pg::resolveFlatTownCandidate(plan, source, -1, -1, 0);
	ASSERT_TRUE(outOfBounds.rejection.has_value());
	EXPECT_EQ(outOfBounds.rejection->category, pg::TownRejectCategory::OutOfBounds);

	const std::vector<pg::PlanBiome> biomes = pg::planBiomesFrom(loadedRegistries().biomes());
	pg::TownBlueprint overlap = source;
	overlap.lots = {source.lots[0], source.lots[1]};
	overlap.lots[1].reservation = overlap.lots[0].reservation;
	const pg::TownContentContext content{.prefabs = loadedRegistries().prefabs(), .town = *biomes.front().town, .worldSeed = 1, .stableEntityId = "claim-test"};
	const auto conflict = pg::resolveFlatTownCandidate(plan, overlap, content, 3, 3, 0);
	ASSERT_TRUE(conflict.rejection.has_value());
	EXPECT_EQ(conflict.rejection->category, pg::TownRejectCategory::ClaimConflict);
	EXPECT_TRUE(plan.placements.empty());
}

TEST(TownCommit, CommitsResolvedArtifactsAtomically)
{
	pg::WorldPlan plan;
	plan.config.size = 4;
	plan.townPath = pg::PlanGrid<std::uint8_t>(4, 0);
	pg::TownCandidate candidate{.blueprintId = "test", .originRow = 1, .originCol = 1, .boundaryCells = {{1,1}}, .pathCells = {{1,1}}};
	pg::commitTownCandidate(plan, candidate);
	ASSERT_EQ(plan.towns.size(), 1u);
	EXPECT_EQ(plan.towns.front().blueprintId, "test");
	EXPECT_EQ(plan.townPath.at(1, 1), 1);
	EXPECT_THROW(pg::commitTownCandidate(plan, candidate), std::logic_error);
	EXPECT_EQ(plan.towns.size(), 1u);
	pg::TownCandidate invalid{.blueprintId="invalid",.originRow=2,.originCol=2,.pathCells={{99,99}}};const pg::TownMutationSnapshot before=pg::snapshotTownMutation(plan);EXPECT_THROW(pg::commitTownCandidate(plan,invalid),std::out_of_range);EXPECT_TRUE(pg::matchesTownMutationSnapshot(plan,before));
}

TEST(StairPlanner, PlanningIsPureAndRepeatable)
{
	const pg::Registries &registries = loadedRegistries();
	pg::WorldPlan plan;
	const pg::StairRequest request{.placements = {{.prefabId = "stair-platform", .anchor = {0, 4, 0}, .foundation = true}}};
	const pg::StairPlanningContext context{.plan = plan, .prefabs = registries.prefabs()};
	const auto first = pg::planStair(request, context);
	const auto second = pg::planStair(request, context);
	ASSERT_TRUE(first.has_value()); ASSERT_TRUE(second.has_value());
	EXPECT_EQ(first->claims, second->claims);
	EXPECT_TRUE(plan.placements.empty());
	EXPECT_TRUE(plan.stairways.empty());
}

TEST(TownPlanner, AuthoredGymTerraceUsesThePureStairPlanner)
{
	const pg::Registries &registries=loadedRegistries(); pg::WorldPlan plan; plan.config.size=16;plan.land=pg::PlanGrid<std::uint8_t>(16,1);plan.water=pg::PlanGrid<std::uint8_t>(16,0);plan.height=pg::PlanGrid<std::int8_t>(16,2);plan.townPath=pg::PlanGrid<std::uint8_t>(16,0);for(int row=7;row<16;++row)for(int col=0;col<16;++col)plan.height.at(row,col)=3;
	const std::vector<pg::PlanBiome> biomes=pg::planBiomesFrom(registries.biomes());const pg::PlanBiome &biome=biomes.front();ASSERT_TRUE(biome.town);const pg::TownMutationSnapshot before=pg::snapshotTownMutation(plan);const auto result=pg::resolveFlatTownCandidate(plan,registries.townBlueprints().get("gym-terrace"),{.prefabs=registries.prefabs(),.town=*biome.town,.worldSeed=44,.stableEntityId="terrace",.roadStairPrefabs=&biome.roadStairLengths},3,3,0);ASSERT_TRUE(result.candidate.has_value()) << (result.rejection?result.rejection->componentId:"");ASSERT_EQ(result.candidate->stairs.size(),1u);EXPECT_EQ(result.candidate->stairs.front().record.steps,1);EXPECT_TRUE(pg::matchesTownMutationSnapshot(plan,before));
}

TEST(WorldPlanValidation, SettlementsComposeTheirRequiredTownBuildings)
{
	const pg::Registries &registries = loadedRegistries();
	Config config;
	config.masterSeed = 42;
	const pg::WorldPlan plan = pg::generateWorldPlan(
		config,
		pg::planBiomesFrom(registries.biomes()),
		registries.placementRules(),
		registries.prefabs(),
		registries.townBlueprints(),
		registries.interiors());

	int settlements = 0;
	int gyms = 0;
	int ports = 0;
	for (const pg::PlanEntity &entity : plan.entities)
	{
		const bool settlement = entity.kind == pg::PlanEntityKind::City || entity.kind == pg::PlanEntityKind::Gym ||
			entity.kind == pg::PlanEntityKind::PortCity;
		settlements += settlement ? 1 : 0;
		gyms += entity.kind == pg::PlanEntityKind::Gym ? 1 : 0;
		ports += entity.kind == pg::PlanEntityKind::PortCity ? 1 : 0;
	}
	const auto roleCount = [&](pg::TownBuildingRole p_role) {
		return static_cast<int>(std::ranges::count_if(plan.placements, [&](const pg::PrefabPlacement &placement) {
			return placement.townRole == p_role;
		}));
	};
	EXPECT_EQ(roleCount(pg::TownBuildingRole::CreatureCenter), settlements);
	EXPECT_EQ(roleCount(pg::TownBuildingRole::Shop), settlements);
	EXPECT_EQ(roleCount(pg::TownBuildingRole::Gym), gyms);
	EXPECT_EQ(roleCount(pg::TownBuildingRole::Port), ports);
	EXPECT_GT(roleCount(pg::TownBuildingRole::Home), 0);
	for (const pg::PrefabPlacement &placement : plan.placements)
	{
		if (placement.townRole == pg::TownBuildingRole::None)
		{
			continue;
		}
		const pg::PrefabDefinition &prefab = registries.prefabs().get(placement.prefabId);
		const pg::ResolvedPlacementBox box = pg::resolvePlacement(prefab.prefab, placement);
		const pg::PrefabAnchor *door = prefab.tryAnchor("door");
		ASSERT_NE(door, nullptr);
		const spk::Vector3Int doorWorld = box.destination + spk::rotateQuarterTurns(
			door->position - prefab.prefab.pivot(), spk::quarterTurnsOf(placement.orientation));
		const int doorRow = plan.cellIndexFromWorld(doorWorld.z);
		const int doorCol = plan.cellIndexFromWorld(doorWorld.x);
		const int terrainSurface = plan.surfaceY(plan.height.at(doorRow, doorCol));
		EXPECT_EQ(doorWorld.y, terrainSurface) << "town door floor must be flush with terrain";
		EXPECT_EQ(box.destination.y, terrainSurface + 1)
			<< "town prefab pivot must remain one block above the terrain surface";
		const int minRow = plan.cellIndexFromWorld(box.worldMin.z);
		const int maxRow = plan.cellIndexFromWorld(box.worldMin.z + box.extents.z - 1);
		const int minCol = plan.cellIndexFromWorld(box.worldMin.x);
		const int maxCol = plan.cellIndexFromWorld(box.worldMin.x + box.extents.x - 1);
		for (int row = minRow; row <= maxRow; ++row)
		{
			for (int col = minCol; col <= maxCol; ++col)
			{
				EXPECT_NE(plan.land.at(row, col), 0);
				EXPECT_EQ(plan.water.at(row, col), 0) << "town prefab must not occupy a river or lake";
				EXPECT_EQ(plan.height.at(row, col), plan.height.at(minRow, minCol))
					<< "town prefab must stand on level terrain";
			}
		}
	}

	int pavedTownCells = 0;
	for (int row = 0; row < config.size; ++row)
	{
		for (int col = 0; col < config.size; ++col)
		{
			if (plan.townPath.at(row, col) != 0)
			{
				++pavedTownCells;
				EXPECT_NE(plan.land.at(row, col), 0);
				EXPECT_EQ(plan.water.at(row, col), 0) << "town path must not pave a river or lake";
			}
		}
	}
	EXPECT_GT(pavedTownCells, 0);
	std::set<std::size_t> macroOwners;
	for(const pg::PlanTownRecord &town:plan.towns)
	{
		EXPECT_TRUE(macroOwners.insert(town.macroEntityIndex).second);
		for(const auto &[row,col]:town.pathCells) EXPECT_NE(plan.townPath.at(row,col),0);
		if(town.kind==pg::PlanEntityKind::PortCity){EXPECT_TRUE(town.boardingEndpoint.has_value());for(std::size_t link:town.boatLinkIndices)EXPECT_LT(link,plan.boatLinks.size());}
		int lamps = 0;
		int benches = 0;
		const std::set<std::pair<int, int>> townPathCells(town.pathCells.begin(), town.pathCells.end());
		const auto isTownPathSurface = [&](int worldX, int worldZ) {
			const int row = plan.cellIndexFromWorld(worldZ);
			const int col = plan.cellIndexFromWorld(worldX);
			if (!townPathCells.contains({row, col}))
			{
				return false;
			}
			const int localX = worldX - (plan.worldOffset() + col * config.blocksPerCell);
			const int localZ = worldZ - (plan.worldOffset() + row * config.blocksPerCell);
			return pg::isCenteredPathSurface(
				config.blocksPerCell, localX, localZ,
				townPathCells.contains({row - 1, col}), townPathCells.contains({row + 1, col}),
				townPathCells.contains({row, col - 1}), townPathCells.contains({row, col + 1}));
		};
		for (const pg::PrefabPlacement &placement : plan.placements)
		{
			if (placement.prefabId != "town-light-pole" && placement.prefabId != "town-bench")
			{
				continue;
			}
			const std::pair<int, int> cell{
				plan.cellIndexFromWorld(placement.anchor.z), plan.cellIndexFromWorld(placement.anchor.x)};
			if (std::ranges::find(town.boundaryCells, cell) == town.boundaryCells.end())
			{
				continue;
			}
			lamps += placement.prefabId == "town-light-pole";
			benches += placement.prefabId == "town-bench";
			const pg::ResolvedPlacementBox decorBox =
				pg::resolvePlacement(registries.prefabs().get(placement.prefabId).prefab, placement);
			bool touchesPath = false;
			for (int z = decorBox.worldMin.z; z < decorBox.worldMin.z + decorBox.extents.z; ++z)
			{
				for (int x = decorBox.worldMin.x; x < decorBox.worldMin.x + decorBox.extents.x; ++x)
				{
					EXPECT_FALSE(isTownPathSurface(x, z)) << placement.prefabId << " overlaps the town path";
					touchesPath = touchesPath || isTownPathSurface(x - 1, z) || isTownPathSurface(x + 1, z) ||
						isTownPathSurface(x, z - 1) || isTownPathSurface(x, z + 1);
				}
			}
			EXPECT_TRUE(touchesPath) << placement.prefabId << " is not beside the rendered town path";
		}
		EXPECT_GE(lamps, 1) << "town " << town.macroEntityIndex << " (" << town.blueprintId << ") has no roadside lamp";
		EXPECT_GE(benches, 1) << "town " << town.macroEntityIndex << " (" << town.blueprintId << ") has no roadside bench";
	}
	EXPECT_EQ(macroOwners.size(),plan.towns.size());
}

TEST(WorldPlanValidation, UsesRoadSwitchbackBeforePerpendicularFallback)
{
	const pg::Registries &registries = loadedRegistries();
	Config config;
	config.masterSeed = 9788;
	const pg::WorldPlan plan = pg::generateWorldPlan(
		config,
		pg::planBiomesFrom(registries.biomes()),
		registries.placementRules(),
		registries.prefabs(),
		registries.townBlueprints(),
		registries.interiors());

	const auto switchback = std::ranges::find_if(plan.stairways, [](const pg::PlanStairway &p_stairway) {
		return p_stairway.road && p_stairway.layout == pg::StairLayout::Switchback;
	});
	ASSERT_NE(switchback, plan.stairways.end());
	EXPECT_EQ(switchback->steps, 3);
	EXPECT_GE(switchback->centerPath.size(), static_cast<std::size_t>(10));
	// A switchback stamps two platforms per landing plus top/bottom: steps + 4
	// contiguous placements, all recorded on the stairway itself.
	EXPECT_EQ(switchback->placementCount, static_cast<std::size_t>(switchback->steps + 4));
	EXPECT_LE(switchback->firstPlacement + switchback->placementCount, plan.placements.size());
	EXPECT_TRUE(switchback->pavedApproach.has_value());
	EXPECT_FALSE(switchback->footprints.empty());
}

TEST(WorldPlanValidation, RepresentativeMinimumConfigurationGenerates)
{
	const pg::Registries &registries = loadedRegistries();
	Config config;
	config.size = Config::MinimumPlanSize;
	config.zoneCount = 1;
	config.maxHeightLevel = 1;
	config.maxRoadStairLevels = 1;
	config.maxWildStairLevels = 1;
	config.maxComposedStairLevels = 1;
	config.lakeMinSize = 1;
	config.lakeMaxSize = config.size * config.size;
	config.gymsPerZone = 0;
	config.citiesPerZone = 0;
	config.normalPoiPerZone = 0;
	config.uncommonPoiPerZone = 0;
	config.rarePoiPerZone = 0;
	config.portsPerContinent = 0;
	config.wildStairsPerZone = 0;

	EXPECT_NO_THROW({
		const pg::WorldPlan plan = pg::generateWorldPlan(
			config,
			pg::planBiomesFrom(registries.biomes()),
			registries.placementRules(),
			registries.prefabs(),
			registries.townBlueprints(),
			registries.interiors());
		EXPECT_EQ(plan.config.size, Config::MinimumPlanSize);
		EXPECT_EQ(plan.land.size(), Config::MinimumPlanSize);
	});
}

TEST(WorldPlanValidation, CoastalLandKeepsItsBiomeHeight)
{
	const pg::Registries &registries = loadedRegistries();
	Config config;
	config.size = Config::MinimumPlanSize;
	config.zoneCount = 1;
	config.coastTrendWeight = 0.0;
	config.undulationLevels = 0.0;
	config.enableRivers = false;
	config.gymsPerZone = 0;
	config.citiesPerZone = 0;
	config.normalPoiPerZone = 0;
	config.uncommonPoiPerZone = 0;
	config.rarePoiPerZone = 0;
	config.portsPerContinent = 0;
	config.wildStairsPerZone = 0;

	pg::PlanBiome biome;
	biome.id = "elevated_coast";
	biome.displayName = "Elevated coast";
	biome.heightShift = 3.0;
	const pg::WorldPlan plan = pg::generateWorldPlan(
		config,
		{biome},
		registries.placementRules(),
		registries.prefabs(),
		registries.townBlueprints(),
		registries.interiors());

	bool foundCoastalLand = false;
	for (int row = 0; row < config.size; ++row)
	{
		for (int col = 0; col < config.size; ++col)
		{
			if (plan.land.at(row, col) == 0)
			{
				continue;
			}
			const bool touchesOcean =
				(row > 0 && plan.land.at(row - 1, col) == 0) ||
				(row + 1 < config.size && plan.land.at(row + 1, col) == 0) ||
				(col > 0 && plan.land.at(row, col - 1) == 0) ||
				(col + 1 < config.size && plan.land.at(row, col + 1) == 0);
			if (touchesOcean)
			{
				foundCoastalLand = true;
				EXPECT_GE(plan.height.at(row, col), 3)
					<< "Coastal biome height was flattened at (" << row << ", " << col << ')';
			}
		}
	}
	EXPECT_TRUE(foundCoastalLand);
}
