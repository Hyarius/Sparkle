#include "world/generator/world_plan_generator.hpp"

#include "world/biome_definition.hpp"
#include "world/generator/placement_rules.hpp"
#include "world/generator/town_planner.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Port of the topology-first Python prototype (worldgen-2.py): the same stages run in
// the same order with the same tunables, but on plain C++ grids. Randomness is drawn
// from generators seeded by hash(masterSeed, semanticPath) so adding a stage never
// reshuffles the output of existing stages. Outputs are not bit-identical to the
// Python previewer (different RNG engine), only structurally equivalent.
//
// This file owns the orchestration; the stage implementations live in the topic
// files listed in world_plan_generator.hpp.

namespace pg::worldgen
{
	Generator::Generator(
		const WorldGenConfig &p_config,
		const std::vector<PlanBiome> &p_biomes,
		const PlanPlacementRules &p_placementRules,
		const Registry<PrefabDefinition> &p_prefabs,
		const TownCompositionCatalog &p_townCompositions,
		const Registry<InteriorDefinition> &p_interiors) :
		cfg(p_config),
		placementRules(p_placementRules),
		prefabs(p_prefabs),
		townCompositions(p_townCompositions),
		interiors(p_interiors),
		size(p_config.size)
	{
		if (p_biomes.empty())
		{
			throw std::runtime_error(
				"world generation needs at least one biome definition with a \"worldgen\" block");
		}
		plan.config = cfg;
		plan.biomes = p_biomes;
		for (const PlanBiome &biome : plan.biomes)
		{
			if (!std::isfinite(biome.townDistanceCells) || biome.townDistanceCells <= 0.0)
			{
				throw std::invalid_argument("world generation biome '" + biome.id + "' needs a positive townDistanceCells");
			}
		}
		plan.land = Mask(size, 0);
		plan.zone = PlanGrid<std::int16_t>(size, -1);
		plan.height = PlanGrid<std::int8_t>(size, -1);
		plan.water = Mask(size, 0);
		plan.lake = Mask(size, 0);
		plan.road = Mask(size, 0);
		plan.townPath = Mask(size, 0);
		plan.bridge = Mask(size, 0);
	}

	// ---------------- Stage H: validation ----------------
	void Generator::computeStats()
	{
		WorldPlanStats &stats = plan.stats;
		stats.continents = continentCount;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				stats.landCells += isLand(i, j) ? 1 : 0;
				stats.roadCells += plan.road.at(i, j) != 0 ? 1 : 0;
				stats.riverCells += (plan.water.at(i, j) != 0 && plan.lake.at(i, j) == 0 && isLand(i, j)) ? 1 : 0;
			}
		}
		for (const PlanEntity &entity : plan.entities)
		{
			if (entity.kind == PlanEntityKind::Gym && distOcean.at(entity.row, entity.col) <= cfg.coastDistCells)
			{
				++stats.gymOnCoast;
			}
		}
		stats.roadDiagonalSteps = countDiagonalOnly(plan.road);
		for (int i = 0; i + 1 < size; ++i)
		{
			for (int j = 0; j + 1 < size; ++j)
			{
				if (plan.road.at(i, j) != 0 && plan.road.at(i, j + 1) != 0 &&
					plan.road.at(i + 1, j) != 0 && plan.road.at(i + 1, j + 1) != 0)
				{
					++stats.roadSquares;
				}
			}
		}
		Mask landWater(size, 0);
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				landWater.at(i, j) = (plan.water.at(i, j) != 0 && isLand(i, j)) ? 1 : 0;
			}
		}
		stats.riverDiagonalSteps = countDiagonalOnly(landWater);
		for (const PlanGateway &gateway : plan.gateways)
		{
			(gateway.primary ? stats.primaryGateways : stats.secondaryGateways) += 1;
		}
		stats.boatLinks = static_cast<int>(plan.boatLinks.size());
		if (stats.roadComponents == 0)
		{
			int componentCount2 = 0;
			(void)labelComponents(plan.road, componentCount2);
			stats.roadComponents = componentCount2;
		}
	}

	WorldPlan Generator::run() &&
	{
		const auto requireTownSiteIds = [&] {
			for (const PlanTownSite &site : plan.townSites)
				if (site.compositionId.empty()) throw std::logic_error("town site composition id was lost between generation stages");
		};
		buildWorldGraph();
		buildLandmass();
		assignZones();
		assignHeights();
		generateWater();
		resolveGateways();
		placeEntities();
		reserveTownSites();
		requireTownSiteIds();
		buildRoads();
		requireTownSiteIds();
		buildTownSpines();
		requireTownSiteIds();
		addBoatLinks();
		requireTownSiteIds();
		markBridges();
		requireTownSiteIds();
		prefabPickRng = rngFor("world/prefab_picks");
		requireTownSiteIds();
		planAndCommitTowns();
		placeStairways();
		placeBuildings();
		placeWildStairways();
		placeScenery();
		computeStats();
		validateWorldPlanTowns(plan);
		return std::move(plan);
	}
}

namespace pg
{
	std::vector<PlanBiome> planBiomesFrom(const Registry<BiomeDefinition> &p_biomes)
	{
		std::vector<PlanBiome> result;
		for (const std::string &id : p_biomes.ids())
		{
			const BiomeDefinition &definition = p_biomes.get(id);
			if (!definition.worldgen.has_value())
			{
				continue; // interior biomes (caves, ...) never enter world generation
			}
			PlanBiome biome;
			biome.id = definition.id;
			biome.displayName = definition.displayName;
			biome.heightShift = definition.worldgen->heightShift;
			biome.peak = definition.worldgen->peak;
			biome.mapColor = definition.worldgen->mapColor;
			biome.roadStairLengths = definition.worldgen->roadStairLengths;
			biome.roadStairPlatform = definition.worldgen->roadStairPlatform;
			biome.wildSlopeLengths = definition.worldgen->wildSlopeLengths;
			biome.wildSlopePlatform = definition.worldgen->wildSlopePlatform;
			biome.wildStairsConfigured = definition.worldgen->wildStairs.configured;
			biome.wildStairAllowCrossZone = definition.worldgen->wildStairs.allowCrossZone;
			biome.wildStairsMaxPerZone = definition.worldgen->wildStairs.maxPerZone;
			biome.wildStairMaxLevels = definition.worldgen->wildStairs.maxLevels;
			biome.wildStairSpacingCells = definition.worldgen->wildStairs.spacingCells;
			biome.wildStairCandidateRatio = definition.worldgen->wildStairs.candidateRatio;
			biome.townDistanceCells = definition.worldgen->towns.distanceCells;
			biome.requiresPort = definition.worldgen->towns.requiresPort;
			for (const BiomeScenery &scenery : definition.worldgen->scenery)
			{
				biome.scenery.push_back({.prefabId = scenery.prefabId, .density = scenery.density, .spacing = scenery.spacing, .prefabSize = scenery.prefabSize});
			}
			for (const BiomeScenery &scenery : definition.worldgen->townScenery)
			{
				biome.townScenery.push_back({.prefabId = scenery.prefabId, .density = scenery.density, .spacing = scenery.spacing, .prefabSize = scenery.prefabSize, .roadside = scenery.roadside});
			}
			for (const auto &[slot, pool] : definition.worldgen->prefabs)
			{
				for (const auto &[key, kind] : planEntityKeyTable())
				{
					if (slot == key)
					{
						biome.entityPrefabs.emplace(kind, pool);
						break;
					}
				}
			}
			if (definition.worldgen->town.has_value())
			{
				const BiomeTown &town = *definition.worldgen->town;
				biome.town = PlanTown{.creatureCenter = town.creatureCenter,
					.shop = town.shop,
					.gym = town.gym,
					.port = town.port,
					.homes = town.homes};
			}
			result.push_back(std::move(biome));
		}
		return result;
	}

	std::string WorldPlan::report() const
	{
		std::ostringstream out;
		const auto count = [&](PlanEntityKind p_kind) {
			int total = 0;
			for (const PlanEntity &entity : entities)
			{
				total += entity.kind == p_kind ? 1 : 0;
			}
			return total;
		};
		out << "================================================================\n";
		out << "WORLD REPORT  seed=" << config.masterSeed << "  size=" << config.size << "x" << config.size << " cells ("
			<< config.size * config.blocksPerCell << " blocks)\n";
		out << "================================================================\n";
		out << "zones................ " << zones.size() << "\n";
		for (const PlanZone &zone : zones)
		{
			out << "  Z" << zone.id << " -> " << biomes[zone.biomeIndex].displayName << " (progression "
				<< zone.progression << ")\n";
		}
		out << "continents........... " << stats.continents << "\n";
		out << "land cells........... " << stats.landCells << "\n";
		out << "river cells.......... " << stats.riverCells << "\n";
		out << "----------------------------------------------------------------\n";
		out << "gym cities........... " << count(PlanEntityKind::Gym) << "\n";
		out << "cities............... " << count(PlanEntityKind::City) << "\n";
		out << "port cities.......... " << count(PlanEntityKind::PortCity) << "\n";
		out << "rare POIs............ " << count(PlanEntityKind::RarePoi) << "\n";
		out << "uncommon POIs........ " << count(PlanEntityKind::UncommonPoi) << "\n";
		out << "normal POIs.......... " << count(PlanEntityKind::NormalPoi) << "\n";
		out << "----------------------------------------------------------------\n";
		out << "road cells........... " << stats.roadCells << "\n";
		out << "road components...... " << stats.roadComponents << "\n";
		out << "road 2x2 squares..... " << stats.roadSquares << "  (MUST be 0)\n";
		out << "boat links........... " << stats.boatLinks << "\n";
		out << "logically connected.. " << ((stats.roadComponents - stats.boatLinks) <= 1 ? "true" : "FALSE") << "  (want true)\n";
		out << "gateways (primary)... " << stats.primaryGateways << "\n";
		out << "gateways (secondary). " << stats.secondaryGateways << "\n";
		out << "stair prefabs........ " << stats.stairPlacements << "\n";
		const auto wildStairwayCount =
			std::ranges::count_if(stairways, [](const PlanStairway &p_stairway) { return !p_stairway.road; });
		out << "wild stair prefabs... " << stats.wildStairPlacements << " (" << wildStairwayCount << " stairways)\n";
		out << "wild candidates...... " << stats.wildStairCandidates << "  (ratio skips "
			<< stats.wildStairRatioSkips << ", spacing skips " << stats.wildStairSpacingSkips
			<< ", placement rejects " << stats.wildStairPlacementRejects << ")\n";
		out << "composed stairways... " << stats.composedStairPlacements << "\n";
		out << "rejected stairways... " << stats.rejectedStairways << "\n";
		out << "scenery prefabs...... " << stats.sceneryPlacements << "\n";
		out << "interiors composed... " << stats.interiorCount << " (" << stats.interiorRoomPlacements
			<< " rooms, " << portals.size() << " portals)\n";
		out << "placement conflicts.. " << stats.placementConflicts << " (" << stats.skippedPoiPlacements
			<< " POIs skipped)\n";
		out << "prefab placements.... " << placements.size() << "\n";
		out << "----------------------------------------------------------------\n";
		out << "gym on coast......... " << stats.gymOnCoast << "  (inland fallback only)\n";
		out << "road diagonal steps.. " << stats.roadDiagonalSteps << "  (MUST be 0)\n";
		out << "river diagonal steps. " << stats.riverDiagonalSteps << "  (MUST be 0)\n";
		if (!stats.warnings.empty())
		{
			out << "WARNINGS:\n";
			for (const std::string &warning : stats.warnings)
			{
				out << "  ! " << warning << "\n";
			}
		}
		out << "================================================================\n";
		return out.str();
	}

	WorldPlan generateWorldPlan(
		const WorldGenConfig &p_config,
		const std::vector<PlanBiome> &p_biomes,
		const PlanPlacementRules &p_placementRules,
		const Registry<PrefabDefinition> &p_prefabs,
		const TownCompositionCatalog &p_townCompositions,
		const Registry<InteriorDefinition> &p_interiors)
	{
		// This is the public allocation boundary. Keep validation ahead of Generator's
		// plan-grid construction and every arithmetic-heavy generation stage.
		validateWorldGenConfig(p_config);
		for(int attempt=0;;++attempt)
		{
			WorldGenConfig resolved=p_config;
			if(attempt>0)resolved.masterSeed=worldgen::deriveSeed(p_config.masterSeed,"world/town_retry/"+std::to_string(attempt));
			try
			{
				WorldPlan plan=worldgen::Generator(resolved,p_biomes,p_placementRules,p_prefabs,p_townCompositions,p_interiors).run();
				if(attempt>0)plan.stats.warnings.push_back("requested seed "+std::to_string(p_config.masterSeed)+" required "+std::to_string(attempt)+" deterministic town-generation retry/retries; resolved seed="+std::to_string(resolved.masterSeed));
				return plan;
			}
			catch(const TownPlanningError &)
			{
				if(attempt>=p_config.maxTownWorldRetries)throw;
			}
		}
	}
}
