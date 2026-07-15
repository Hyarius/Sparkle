#include "core/paths.hpp"
#include "core/registries.hpp"
#include "world/generator/plan_chunk_provider.hpp"
#include "world/generator/town_commit.hpp"
#include "world/generator/town_composition.hpp"
#include "world/generator/town_planner.hpp"
#include "world/generator/town_workspace.hpp"
#include "world/generator/world_plan.hpp"
#include "world/prefab_placement_math.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace
{
	const pg::Registries &loadedRegistries()
	{
		static const std::unique_ptr<pg::Registries> registries = [] {
			auto value = std::make_unique<pg::Registries>();
			value->loadAll(pg::resourceRoot() / "data");
			return value;
		}();
		return *registries;
	}

	pg::WorldPlan flatTownPlan()
	{
		pg::WorldPlan plan;
		plan.config.size = 64;
		plan.config.blocksPerCell = 8;
		plan.config.blocksPerLevel = 3;
		plan.land = pg::PlanGrid<std::uint8_t>(64, 1);
		plan.water = pg::PlanGrid<std::uint8_t>(64, 0);
		plan.lake = pg::PlanGrid<std::uint8_t>(64, 0);
		plan.height = pg::PlanGrid<std::int8_t>(64, 0);
		plan.zone = pg::PlanGrid<std::int16_t>(64, 0);
		plan.road = pg::PlanGrid<std::uint8_t>(64, 0);
		plan.townPath = pg::PlanGrid<std::uint8_t>(64, 0);
		plan.bridge = pg::PlanGrid<std::uint8_t>(64, 0);
		for (int col = 22; col <= 42; ++col)
		{
			plan.road.at(32, col) = 1;
		}
		plan.entities.push_back({.kind = pg::PlanEntityKind::City, .row = 32, .col = 32, .zone = 0, .continent = 1});
		plan.zones.push_back({.id = 0, .biomeIndex = 0, .progression = 0});
		plan.biomes = pg::planBiomesFrom(loadedRegistries().biomes());
		return plan;
	}
}

TEST(TownComposition, ShippedCatalogHasStableKindsAndSchema)
{
	const pg::TownCompositionCatalog &catalog = loadedRegistries().townCompositions();
	EXPECT_EQ(catalog.ids(), (std::vector<std::string>{"city", "gym", "port"}));
	const pg::TownComposition &city = catalog.get("city");
	EXPECT_EQ(city.kind, pg::TownCompositionKind::City);
	EXPECT_TRUE(city.layout.urbanRoadWidth % 2 == 1);
	EXPECT_EQ(city.buildings.front().count.minimum, 1);
}

TEST(BiomeTownDistribution, ShippedBiomesProvideSpacingAndTheCoastRequiresAPort)
{
	const std::vector<pg::PlanBiome> biomes = pg::planBiomesFrom(loadedRegistries().biomes());
	ASSERT_FALSE(biomes.empty());
	for (const pg::PlanBiome &biome : biomes)
	{
		EXPECT_GT(biome.townDensityDistanceCells, 0.0) << biome.id;
		EXPECT_GT(biome.minimumTownDistanceCells, 0.0) << biome.id;
		EXPECT_LE(biome.minimumTownDistanceCells, biome.townDensityDistanceCells) << biome.id;
	}
	const auto coast = std::find_if(biomes.begin(), biomes.end(), [](const pg::PlanBiome &biome) {
		return biome.id == "coast";
	});
	ASSERT_NE(coast, biomes.end());
	EXPECT_TRUE(coast->requiresPort);
}

TEST(WorldGeneration, ReservesEveryGymAndConfiguredBiomePort)
{
	const pg::Registries &registries = loadedRegistries();
	for (const std::uint64_t seed : {std::uint64_t{1}, std::uint64_t{97844}})
	{
		pg::WorldGenConfig config;
		config.masterSeed = seed;
		config.size = 248;
		const pg::WorldPlan plan = pg::generateWorldPlan(
			config,
			pg::planBiomesFrom(registries.biomes()),
			registries.placementRules(),
			registries.prefabs(),
			registries.townCompositions(),
			registries.interiors());
		EXPECT_FALSE(std::ranges::any_of(plan.stats.warnings, [](const std::string &warning) {
			return warning.contains("settlement target");
		})) << seed;
		for (const pg::PlanZone &zone : plan.zones)
		{
			const pg::PlanBiome &biome = plan.biomes[zone.biomeIndex];
			int gyms = 0;
			int ports = 0;
			for (const pg::PlanEntity &entity : plan.entities)
			{
				if (entity.zone != zone.id)
				{
					continue;
				}
				gyms += entity.kind == pg::PlanEntityKind::Gym ? 1 : 0;
				ports += entity.kind == pg::PlanEntityKind::PortCity ? 1 : 0;
			}
			EXPECT_EQ(gyms, 1) << biome.id << " seed " << seed;
			EXPECT_EQ(ports, biome.requiresPort ? 1 : 0) << biome.id << " seed " << seed;
		}
		for (const pg::PlanTownRecord &town : plan.towns)
		{
			const pg::PlanEntity &owner = plan.entities[town.macroEntityIndex];
			if (town.kind != pg::PlanEntityKind::PortCity)
			{
				const int firstRow = plan.cellIndexFromWorld(town.bounds.minZ);
				const int lastRow = plan.cellIndexFromWorld(town.bounds.maxZ);
				const int firstCol = plan.cellIndexFromWorld(town.bounds.minX);
				const int lastCol = plan.cellIndexFromWorld(town.bounds.maxX);
				for (int row = firstRow; row <= lastRow; ++row)
				{
					for (int col = firstCol; col <= lastCol; ++col)
					{
						if (plan.land.at(row, col) != 0 && plan.water.at(row, col) == 0)
						{
							EXPECT_EQ(plan.zone.at(row, col), owner.zone) << town.compositionId << " seed " << seed;
						}
					}
				}
			}
			std::set<std::size_t> townPlacementIndices;
			townPlacementIndices.insert(town.buildingPlacementIndices.begin(), town.buildingPlacementIndices.end());
			townPlacementIndices.insert(town.roadSceneryPlacementIndices.begin(), town.roadSceneryPlacementIndices.end());
			townPlacementIndices.insert(town.groundSceneryPlacementIndices.begin(), town.groundSceneryPlacementIndices.end());
			for (const pg::PlanStairway &stairway : plan.stairways)
			{
				for (std::size_t index = stairway.firstPlacement; index < stairway.firstPlacement + stairway.placementCount; ++index)
				{
					townPlacementIndices.insert(index);
				}
			}
			for (std::size_t index = 0; index < plan.placements.size(); ++index)
			{
				if (townPlacementIndices.contains(index))
				{
					continue;
				}
				const pg::PrefabPlacement &placement = plan.placements[index];
				const pg::PrefabDefinition *prefab = registries.prefabs().tryGet(placement.prefabId);
				ASSERT_NE(prefab, nullptr) << placement.prefabId;
				const pg::ResolvedPlacementBox box = pg::resolvePlacement(prefab->prefab, placement);
				const bool overlapsTown = box.worldMin.x <= town.bounds.maxX &&
										  box.worldMin.x + box.extents.x - 1 >= town.bounds.minX &&
										  box.worldMin.z <= town.bounds.maxZ &&
										  box.worldMin.z + box.extents.z - 1 >= town.bounds.minZ;
				EXPECT_FALSE(overlapsTown) << placement.prefabId << " seed " << seed;
			}
		}
	}
}

TEST(WorldGeneration, BuildsTownsForTerrainAndWaterfrontRegressionSeeds)
{
	const pg::Registries &registries = loadedRegistries();
	// These previously exhausted every local town-layout attempt despite having
	// a valid nearby construction site.
	for (const std::uint64_t seed : {std::uint64_t{2}, std::uint64_t{16}, std::uint64_t{84985}, std::uint64_t{849854}, std::uint64_t{8498545}})
	{
		pg::WorldGenConfig config;
		config.masterSeed = seed;
		config.size = 248;
		EXPECT_NO_THROW(static_cast<void>(pg::generateWorldPlan(config, pg::planBiomesFrom(registries.biomes()), registries.placementRules(), registries.prefabs(), registries.townCompositions(), registries.interiors()))) << seed;
	}
}

TEST(TownWorkspace, ConvertsWorldColumnsAndProjectsTheSharedMacroRoadSurface)
{
	pg::WorldPlan plan = flatTownPlan();
	plan.height.at(32, 32) = 1;
	pg::TownComposition composition = loadedRegistries().townCompositions().get("city");
	pg::PlanTownSite site{.macroEntityIndex = 0, .kind = pg::PlanEntityKind::City, .compositionId = "city", .centerRow = 32, .centerCol = 32, .radiusColumns = 16};
	pg::TownWorkspace workspace(plan, site, composition);
	const pg::WorldColumn center{plan.worldOffset() + 32 * 8 + 4, plan.worldOffset() + 32 * 8 + 4};
	EXPECT_EQ(workspace.worldFromTown(workspace.townFromWorld(center)), center);
	EXPECT_EQ(workspace.planCellFromWorld(center), (pg::PlanCell{32, 32}));
	EXPECT_EQ(workspace.surfaceHeight(workspace.townFromWorld(center)), plan.surfaceY(1));
	EXPECT_FALSE(workspace.mainRoadSurface().empty());
	for (const pg::PlanPavedColumn &column : workspace.mainRoadSurface())
	{
		const pg::PlanCell cell = workspace.planCellFromWorld({column.worldX, column.worldZ});
		EXPECT_NE(plan.road.at(cell.row, cell.col), 0);
	}
}

TEST(TownRealization, KeepsBuildingEntrancesFlushWithTheirPlannedApproachesUnderRelief)
{
	pg::WorldPlan plan = flatTownPlan();
	plan.config.masterSeed = 0x5eed1234ULL;
	plan.config.terrainVariationThreshold = 1.0;
	plan.config.terrainVariationFeatureBlocks = 16.0;
	// Force the reservation across a macro height transition. The town keeps both
	// levels and relies on road stairways where the network changes elevation.
	plan.height.at(31, 32) = 1;
	plan.height.at(32, 32) = 2;
	plan.height.at(33, 32) = 1;
	pg::TownComposition composition = loadedRegistries().townCompositions().get("city");
	composition.layout.buildingAttemptsPerItem = 256;
	const pg::PlanTownSite site{.macroEntityIndex = 0, .kind = pg::PlanEntityKind::City, .compositionId = "city", .centerRow = 32, .centerCol = 32, .radiusColumns = 24};
	const pg::PlanTown &biomeTown = *plan.biomes.front().town;
	const pg::TownPlanResult result = pg::planTown(plan, site, composition, loadedRegistries().prefabs(), biomeTown, 912345);
	ASSERT_TRUE(result.candidate.has_value()) << result.rejection->message;
	pg::commitTownCandidate(plan, *result.candidate);

	const pg::PlanChunkProvider provider(loadedRegistries(), plan);
	const pg::PlanTownRecord &town = plan.towns.front();
	ASSERT_FALSE(town.entrances.empty());
	EXPECT_EQ(provider.surfaceHeight(town.bounds.minX, town.bounds.minZ), plan.surfaceY(0));
	for (const pg::PlanPavedColumn &column : town.mainRoadSurface)
	{
		EXPECT_EQ(provider.surfaceHeight(column.worldX, column.worldZ), column.surfaceY) << column.worldX << "," << column.worldZ;
	}
	for (const pg::PlanPavedColumn &column : town.urbanRoadSurface)
	{
		EXPECT_EQ(provider.surfaceHeight(column.worldX, column.worldZ), column.surfaceY) << column.worldX << "," << column.worldZ;
	}
	for (const pg::ResolvedTownEntranceRecord &entrance : town.entrances)
	{
		EXPECT_EQ(provider.surfaceHeight(entrance.threshold.x, entrance.threshold.z), entrance.threshold.y)
			<< entrance.buildingInstanceId;
	}
}

TEST(TownPlanner, DerivesWorkspaceRadiusFromSeededContent)
{
	pg::WorldPlan plan = flatTownPlan();
	pg::TownComposition composition = loadedRegistries().townCompositions().get("city");
	pg::TownRejection rejection;
	const pg::PlanTown &biomeTown = *plan.biomes.front().town;
	const std::optional<int> radius = pg::deriveTownRadius(composition, loadedRegistries().prefabs(), biomeTown, 2026, 0, rejection);
	ASSERT_TRUE(radius.has_value()) << rejection.message;
	EXPECT_GE(*radius, 20);

	pg::TownComposition larger = composition;
	for (pg::TownBuildingRequest &request : larger.buildings)
	{
		if (request.role == pg::TownBuildingRole::Home)
		{
			request.count = {.minimum = 6, .maximum = 6};
		}
	}
	pg::TownRejection largerRejection;
	const std::optional<int> largerRadius = pg::deriveTownRadius(larger, loadedRegistries().prefabs(), biomeTown, 2026, 0, largerRejection);
	ASSERT_TRUE(largerRadius.has_value()) << largerRejection.message;
	EXPECT_GT(*largerRadius, *radius);
}

TEST(TownPlanner, BuildsAnAtomicProceduralCandidateWithConnectedDoors)
{
	pg::WorldPlan plan = flatTownPlan();
	pg::TownComposition composition = loadedRegistries().townCompositions().get("city");
	composition.layout.layoutAttempts = 64;
	composition.layout.buildingAttemptsPerItem = 256;
	const pg::PlanTown &biomeTown = *plan.biomes.front().town;
	const pg::PlanTownSite site{.macroEntityIndex = 0, .kind = pg::PlanEntityKind::City, .compositionId = "city", .centerRow = 32, .centerCol = 32, .radiusColumns = 24};
	const pg::TownMutationSnapshot before = pg::snapshotTownMutation(plan);
	const pg::TownPlanResult result = pg::planTown(plan, site, composition, loadedRegistries().prefabs(), biomeTown, 912345);
	ASSERT_TRUE(result.candidate.has_value()) << (result.rejection ? result.rejection->message : "");
	EXPECT_TRUE(pg::matchesTownMutationSnapshot(plan, before));
	EXPECT_GE(result.candidate->buildings.size(), 5u);
	EXPECT_EQ(result.candidate->buildings.size(), result.candidate->entrances.size());
	EXPECT_FALSE(result.candidate->urbanRoadSurface.empty());
	for (const pg::ResolvedTownEntrance &entrance : result.candidate->entrances)
	{
		EXPECT_FALSE(entrance.approachColumns.empty());
		EXPECT_NE((std::pair{entrance.threshold.x, entrance.threshold.z}), (std::pair{result.candidate->bounds.minX + entrance.connectionPoint.x, result.candidate->bounds.minZ + entrance.connectionPoint.z}));
		int nearestSpineColumn = 1000000;
		for (const pg::PlanPavedColumn &spine : result.candidate->mainRoadSurface)
		{
			nearestSpineColumn = std::min(nearestSpineColumn, std::abs(entrance.threshold.x - spine.worldX) + std::abs(entrance.threshold.z - spine.worldZ));
		}
		// Doors are sampled from the compact spine band rather than the full town
		// workspace; the direct reserved lane then gives every building a street.
		EXPECT_LE(nearestSpineColumn, 16);
	}
	pg::commitTownCandidate(plan, *result.candidate);
	EXPECT_NO_THROW(pg::validateWorldPlanTowns(plan));
}

TEST(TownPlanner, SameSeedProducesTheSameCandidate)
{
	pg::WorldPlan plan = flatTownPlan();
	pg::TownComposition composition = loadedRegistries().townCompositions().get("city");
	composition.layout.layoutAttempts = 64;
	composition.layout.buildingAttemptsPerItem = 256;
	const pg::PlanTownSite site{.macroEntityIndex = 0, .kind = pg::PlanEntityKind::City, .compositionId = "city", .centerRow = 32, .centerCol = 32, .radiusColumns = 24};
	const pg::PlanTown &biomeTown = *plan.biomes.front().town;
	const pg::TownPlanResult first = pg::planTown(plan, site, composition, loadedRegistries().prefabs(), biomeTown, 77);
	const pg::TownPlanResult second = pg::planTown(plan, site, composition, loadedRegistries().prefabs(), biomeTown, 77);
	ASSERT_TRUE(first.candidate.has_value());
	ASSERT_TRUE(second.candidate.has_value());
	EXPECT_EQ(first.candidate->urbanRoadSurface, second.candidate->urbanRoadSurface);
	EXPECT_EQ(first.candidate->routes.size(), second.candidate->routes.size());
	for (std::size_t index = 0; index < first.candidate->buildings.size(); ++index)
	{
		EXPECT_EQ(first.candidate->buildings[index].prefabId, second.candidate->buildings[index].prefabId);
		EXPECT_EQ(first.candidate->buildings[index].anchor, second.candidate->buildings[index].anchor);
		EXPECT_EQ(first.candidate->buildings[index].orientation, second.candidate->buildings[index].orientation);
	}
}
