#include "core/registries.hpp"
#include "world/generator/town_commit.hpp"
#include "world/generator/town_composition.hpp"
#include "world/generator/town_planner.hpp"
#include "world/generator/town_workspace.hpp"
#include "world/generator/world_plan.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>

namespace
{
	const pg::Registries &loadedRegistries()
	{
		static const std::unique_ptr<pg::Registries> registries = [] {
			auto value = std::make_unique<pg::Registries>();
			value->loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
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
		for (int col = 22; col <= 42; ++col) plan.road.at(32, col) = 1;
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

TEST(TownWorkspace, ConvertsWorldColumnsAndProjectsTheSharedMacroRoadSurface)
{
	pg::WorldPlan plan = flatTownPlan();
	pg::TownComposition composition = loadedRegistries().townCompositions().get("city");
	pg::PlanTownSite site{.macroEntityIndex = 0, .kind = pg::PlanEntityKind::City, .compositionId = "city", .centerRow = 32, .centerCol = 32, .radiusColumns = 16};
	pg::TownWorkspace workspace(plan, site, composition);
	const pg::WorldColumn center{plan.worldOffset() + 32 * 8 + 4, plan.worldOffset() + 32 * 8 + 4};
	EXPECT_EQ(workspace.worldFromTown(workspace.townFromWorld(center)), center);
	EXPECT_EQ(workspace.planCellFromWorld(center), (pg::PlanCell{32, 32}));
	EXPECT_FALSE(workspace.mainRoadSurface().empty());
	for (const pg::PlanPavedColumn &column : workspace.mainRoadSurface())
	{
		const pg::PlanCell cell = workspace.planCellFromWorld({column.worldX, column.worldZ});
		EXPECT_NE(plan.road.at(cell.row, cell.col), 0);
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
		if (request.role == pg::TownBuildingRole::Home)
			request.count = {.minimum = 6, .maximum = 6};
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
