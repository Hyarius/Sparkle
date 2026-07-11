#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/registries.hpp"
#include "world/generator/plan_chunk_provider.hpp"
#include "world/generator/world_plan.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

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

	const std::int32_t terrainId = registries.voxels().numericId("stone-block");
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

TEST(WorldPlanValidation, UsesRoadSwitchbackBeforePerpendicularFallback)
{
	const pg::Registries &registries = loadedRegistries();
	Config config;
	config.masterSeed = 9784;
	const pg::WorldPlan plan = pg::generateWorldPlan(
		config,
		pg::planBiomesFrom(registries.biomes()),
		registries.placementRules(),
		registries.prefabs(),
		registries.interiors());

	const auto switchback = std::ranges::find_if(plan.stairways, [](const pg::PlanStairway &p_stairway) {
		return p_stairway.road && p_stairway.switchback;
	});
	ASSERT_NE(switchback, plan.stairways.end());
	EXPECT_FALSE(switchback->perpendicular);
	EXPECT_EQ(switchback->steps, 4);
	EXPECT_GE(switchback->centerPath.size(), static_cast<std::size_t>(10));
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
