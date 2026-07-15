#include "world/generator/world_plan.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
	[[noreturn]] void invalid(std::string_view p_field, std::string_view p_constraint)
	{
		throw std::invalid_argument(
			"WorldGenConfig." + std::string(p_field) + " " + std::string(p_constraint));
	}

	void require(bool p_condition, std::string_view p_field, std::string_view p_constraint)
	{
		if (!p_condition)
		{
			invalid(p_field, p_constraint);
		}
	}

	void finiteRange(
		std::string_view p_field,
		double p_value,
		double p_minimum,
		double p_maximum,
		bool p_includeMinimum = true,
		bool p_includeMaximum = true)
	{
		if (!std::isfinite(p_value))
		{
			invalid(p_field, "must be finite");
		}
		const bool aboveMinimum = p_includeMinimum ? p_value >= p_minimum : p_value > p_minimum;
		const bool belowMaximum = p_includeMaximum ? p_value <= p_maximum : p_value < p_maximum;
		if (!aboveMinimum || !belowMaximum)
		{
			const std::string interval =
				std::string("must be in ") + (p_includeMinimum ? "[" : "(") +
				std::to_string(p_minimum) + ", " + std::to_string(p_maximum) +
				(p_includeMaximum ? "]" : ")") + "; got " + std::to_string(p_value);
			invalid(p_field, interval);
		}
	}
}

namespace pg
{
	void validateWorldPlanTowns(const WorldPlan &p_plan)
	{
		std::set<std::size_t> owners;
		for (const PlanTownRecord &town : p_plan.towns)
		{
			if (town.macroEntityIndex >= p_plan.entities.size() || !owners.insert(town.macroEntityIndex).second)
			{
				throw std::logic_error("WorldPlan town has an invalid or duplicate macro owner");
			}
			const PlanEntity &entity = p_plan.entities[town.macroEntityIndex];
			if (entity.kind != town.kind || town.compositionId.empty())
			{
				throw std::logic_error("WorldPlan town kind or composition differs from its macro owner");
			}
			std::map<TownBuildingRole, int> roles;
			for (std::size_t index : town.buildingPlacementIndices)
			{
				if (index >= p_plan.placements.size())
				{
					throw std::logic_error("WorldPlan town building index is invalid");
				}
				++roles[p_plan.placements[index].townRole];
			}
			if (roles[TownBuildingRole::CreatureCenter] != 1 || roles[TownBuildingRole::Shop] != 1 ||
				roles[TownBuildingRole::Home] < (town.kind == PlanEntityKind::City ? 3 : 2) ||
				(town.kind == PlanEntityKind::Gym && roles[TownBuildingRole::Gym] != 1) ||
				(town.kind == PlanEntityKind::PortCity && roles[TownBuildingRole::Port] != 1))
			{
				throw std::logic_error("WorldPlan town is missing a required building role");
			}
			if (town.mainRoadSurface.empty())
			{
				throw std::logic_error("WorldPlan town has no projected main-road spine");
			}
			const bool mainTouchesCenter = std::ranges::any_of(town.mainRoadSurface, [&](const PlanPavedColumn &column) {
				return p_plan.cellIndexFromWorld(column.worldZ) == town.centerRow && p_plan.cellIndexFromWorld(column.worldX) == town.centerCol;
			});
			if (!mainTouchesCenter)
			{
				throw std::logic_error("WorldPlan main-road spine does not reach the settlement center");
			}

			std::set<std::pair<int, int>> main;
			std::set<std::pair<int, int>> network;
			for (const PlanPavedColumn &column : town.mainRoadSurface)
			{
				main.emplace(column.worldX, column.worldZ);
				network.emplace(column.worldX, column.worldZ);
			}
			for (const PlanPavedColumn &column : town.urbanRoadSurface)
			{
				if (!network.insert({column.worldX, column.worldZ}).second)
				{
					continue;
				}
				for (const PlanClaim &claim : p_plan.townClaims)
				{
					if (column.worldX >= claim.min.x && column.worldX <= claim.max.x && column.worldZ >= claim.min.z && column.worldZ <= claim.max.z && column.surfaceY >= claim.min.y && column.surfaceY <= claim.max.y)
					{
						throw std::logic_error("WorldPlan urban road intersects a town claim at " + std::to_string(column.worldX) + "," + std::to_string(column.worldZ) + " y=" + std::to_string(column.surfaceY) + " claim=" + std::to_string(claim.min.x) + "," + std::to_string(claim.min.y) + "," + std::to_string(claim.min.z) + ".." + std::to_string(claim.max.x) + "," + std::to_string(claim.max.y) + "," + std::to_string(claim.max.z));
					}
				}
			}
			for (const ResolvedTownEntranceRecord &entrance : town.entrances)
			{
				if (network.contains({entrance.threshold.x, entrance.threshold.z}))
				{
					throw std::logic_error("WorldPlan paved a building-owned door threshold");
				}
				for (const auto &approach : entrance.approachColumns)
				{
					if (network.contains(approach))
					{
						throw std::logic_error("WorldPlan paved a door approach");
					}
				}
				if (!network.contains(entrance.connectionPoint))
				{
					throw std::logic_error("WorldPlan entrance connection point is not in the road network");
				}
				std::set<std::pair<int, int>> seen{entrance.connectionPoint};
				std::vector<std::pair<int, int>> open{entrance.connectionPoint};
				for (std::size_t cursor = 0; cursor < open.size(); ++cursor)
				{
					const auto [x, z] = open[cursor];
					for (const auto [dx, dz] : std::array{std::pair{0, -1}, std::pair{1, 0}, std::pair{0, 1}, std::pair{-1, 0}})
					{
						if (const std::pair<int, int> next{x + dx, z + dz}; network.contains(next) && seen.insert(next).second)
						{
							open.push_back(next);
						}
					}
				}
				if (!std::ranges::any_of(seen, [&](const auto &column) {
						return main.contains(column);
					}))
				{
					throw std::logic_error("WorldPlan entrance cannot reach the global main road");
				}
			}
			if (town.kind == PlanEntityKind::PortCity && (!town.boardingEndpoint || town.dockCells.empty()))
			{
				throw std::logic_error("WorldPlan port town has no dock or boarding endpoint");
			}
			if (town.boardingEndpoint)
			{
				const int row = p_plan.cellIndexFromWorld(town.boardingEndpoint->z), col = p_plan.cellIndexFromWorld(town.boardingEndpoint->x);
				if (!p_plan.land.contains(row, col) || (p_plan.land.at(row, col) != 0 && p_plan.water.at(row, col) == 0) || !std::ranges::contains(town.dockCells, std::pair{row, col}))
				{
					throw std::logic_error("WorldPlan boarding endpoint is not on the water end of its dock");
				}
			}
			for (std::size_t link : town.boatLinkIndices)
			{
				if (link >= p_plan.boatLinks.size())
				{
					throw std::logic_error("WorldPlan port town has an invalid boat link");
				}
			}
		}
		const std::size_t settlements = static_cast<std::size_t>(std::ranges::count_if(p_plan.entities, [](const PlanEntity &entity) {
			return entity.kind == PlanEntityKind::City || entity.kind == PlanEntityKind::Gym || entity.kind == PlanEntityKind::PortCity;
		}));
		if (owners.size() != settlements)
		{
			throw std::logic_error("WorldPlan does not contain exactly one town per settlement");
		}
	}

	void validateWorldGenConfig(const WorldGenConfig &p_config)
	{
		using Config = WorldGenConfig;

		require(
			p_config.size >= Config::MinimumPlanSize && p_config.size <= Config::MaximumPlanSize,
			"size",
			"must be between MinimumPlanSize and MaximumPlanSize");

		const std::int64_t planArea = static_cast<std::int64_t>(p_config.size) * p_config.size;
		require(
			planArea <= std::numeric_limits<int>::max(),
			"size",
			"produces a plan area that cannot be indexed by int");
		require(
			p_config.zoneCount >= 1 && p_config.zoneCount <= Config::MaximumZoneCount &&
				p_config.zoneCount <= planArea,
			"zoneCount",
			"must be positive, bounded, and no greater than the plan area");
		require(
			p_config.zoneCount - 1 <= std::numeric_limits<std::int16_t>::max(),
			"zoneCount",
			"does not fit PlanGrid<int16_t> zone identifiers");

		finiteRange("landThreshold", p_config.landThreshold, 0.0, 1.0, false, false);
		finiteRange("coastNoise", p_config.coastNoise, 0.0, 2.0);
		finiteRange("fragmentation", p_config.fragmentation, 0.0, 1.0);
		finiteRange("minZoneFraction", p_config.minZoneFraction, 0.0, 1.0, false, true);

		require(
			p_config.maxHeightLevel >= 1 && p_config.maxHeightLevel <= Config::MaximumHeightLevel,
			"maxHeightLevel",
			"must be positive and fit PlanGrid<int8_t> heights");
		finiteRange("cellsPerLevel", p_config.cellsPerLevel, 0.0, 4096.0, false, true);
		finiteRange("coastTrendWeight", p_config.coastTrendWeight, 0.0, 1.0);
		finiteRange(
			"undulationLevels",
			p_config.undulationLevels,
			0.0,
			static_cast<double>(Config::MaximumHeightLevel));
		finiteRange("heightFeatureCells", p_config.heightFeatureCells, 0.0, 4096.0, false, true);

		finiteRange(
			"riversPerZone",
			p_config.riversPerZone,
			0.0,
			static_cast<double>(Config::MaximumPerZoneCount));
		finiteRange(
			"lakeMinDepth",
			p_config.lakeMinDepth,
			0.0,
			static_cast<double>(p_config.maxHeightLevel),
			false,
			true);
		require(
			p_config.lakeMinSize >= 1 && p_config.lakeMinSize <= planArea,
			"lakeMinSize",
			"must be between 1 and the plan area");
		require(
			p_config.lakeMaxSize >= 1 && p_config.lakeMaxSize <= planArea,
			"lakeMaxSize",
			"must be between 1 and the plan area");
		require(
			p_config.lakeMinSize <= p_config.lakeMaxSize,
			"lakeMinSize/lakeMaxSize",
			"must be ordered with lakeMinSize <= lakeMaxSize");

		const std::array integerCounts = {
			std::pair{"normalPoiPerZone", p_config.normalPoiPerZone},
			std::pair{"uncommonPoiPerZone", p_config.uncommonPoiPerZone},
			std::pair{"rarePoiPerZone", p_config.rarePoiPerZone},
			std::pair{"poiRoadConnections", p_config.poiRoadConnections},
			std::pair{"wildStairsPerZone", p_config.wildStairsPerZone}};
		for (const auto &[field, value] : integerCounts)
		{
			require(
				value >= 0 && value <= Config::MaximumPerZoneCount,
				field,
				"must be between 0 and MaximumPerZoneCount");
		}

		require(p_config.townSearchRadiusCells >= 0 && p_config.townSearchRadiusCells <= Config::MaximumPlanSize, "townSearchRadiusCells", "must be between 0 and MaximumPlanSize");
		require(p_config.maxTownWorldRetries >= 0 && p_config.maxTownWorldRetries <= 64, "maxTownWorldRetries", "must be between 0 and 64");

		const std::array radii = {
			std::pair{"blockGym", p_config.blockGym},
			std::pair{"blockCity", p_config.blockCity},
			std::pair{"blockRare", p_config.blockRare},
			std::pair{"blockUncommon", p_config.blockUncommon},
			std::pair{"blockNormal", p_config.blockNormal},
			std::pair{"coastDistCells", p_config.coastDistCells},
			std::pair{"wildStairSpacingCells", p_config.wildStairSpacingCells}};
		for (const auto &[field, value] : radii)
		{
			finiteRange(field, value, 0.0, static_cast<double>(p_config.size));
		}

		const std::array ratios = {std::pair{"distRatioPoi", p_config.distRatioPoi}};
		for (const auto &[field, value] : ratios)
		{
			finiteRange(field, value, 0.0, 1.0);
		}

		require(
			p_config.maxComposedStairLevels >= 1 &&
				p_config.maxComposedStairLevels <= p_config.maxHeightLevel,
			"maxComposedStairLevels",
			"must be between 1 and maxHeightLevel");
		require(
			p_config.maxRoadStairLevels >= 1 &&
				p_config.maxRoadStairLevels <= p_config.maxComposedStairLevels,
			"maxRoadStairLevels",
			"must be between 1 and maxComposedStairLevels");
		require(
			p_config.maxWildStairLevels >= 1 &&
				p_config.maxWildStairLevels <= p_config.maxComposedStairLevels,
			"maxWildStairLevels",
			"must be between 1 and maxComposedStairLevels");

		require(
			p_config.blocksPerCell >= 4 && p_config.blocksPerCell <= Config::MaximumBlocksPerCell,
			"blocksPerCell",
			"must be between 4 and MaximumBlocksPerCell");
		require(
			p_config.blocksPerLevel >= 1 &&
				p_config.blocksPerLevel <= Config::MaximumBlocksPerLevel &&
				p_config.blocksPerLevel <= p_config.blocksPerCell,
			"blocksPerLevel",
			"must be positive, bounded, and no greater than blocksPerCell");
		require(
			p_config.groundLevelTop >= -Config::MaximumGroundMagnitude &&
				p_config.groundLevelTop <= Config::MaximumGroundMagnitude,
			"groundLevelTop",
			"is outside the supported world-height range");
		finiteRange(
			"terrainVariationFeatureBlocks",
			p_config.terrainVariationFeatureBlocks,
			0.0,
			4096.0,
			false,
			true);
		require(
			p_config.terrainVariationOctaves >= 1 && p_config.terrainVariationOctaves <= 8,
			"terrainVariationOctaves",
			"must be between 1 and 8");
		finiteRange(
			"terrainVariationPersistence",
			p_config.terrainVariationPersistence,
			0.0,
			1.0,
			false,
			true);
		finiteRange(
			"terrainVariationThreshold",
			p_config.terrainVariationThreshold,
			0.0,
			1.0);
		require(
			p_config.terrainVariationTransitionBlocks >= 1 && p_config.terrainVariationTransitionBlocks <= 8,
			"terrainVariationTransitionBlocks",
			"must be between 1 and 8");
		require(
			p_config.interiorRegionGap >= 0 && p_config.interiorRegionGap <= Config::MaximumInteriorGap,
			"interiorRegionGap",
			"must be between 0 and MaximumInteriorGap");
		require(
			p_config.interiorSlotBlocks >= Config::MinimumInteriorSlotBlocks &&
				p_config.interiorSlotBlocks <= Config::MaximumInteriorSlotBlocks,
			"interiorSlotBlocks",
			"is outside the supported slot-size range");

		const std::int64_t worldWidth = static_cast<std::int64_t>(p_config.size) * p_config.blocksPerCell;
		require(
			worldWidth <= std::numeric_limits<int>::max(),
			"size/blocksPerCell",
			"produce a world extent outside the int coordinate range");
		const std::int64_t maximumSurface = static_cast<std::int64_t>(p_config.groundLevelTop) +
											static_cast<std::int64_t>(p_config.maxHeightLevel) * p_config.blocksPerLevel;
		require(
			maximumSurface >= std::numeric_limits<int>::min() &&
				maximumSurface <= std::numeric_limits<int>::max(),
			"groundLevelTop/maxHeightLevel/blocksPerLevel",
			"produce a surface height outside the int coordinate range");
		const std::int64_t interiorMinX = worldWidth / 2 + p_config.interiorRegionGap;
		require(
			interiorMinX <= std::numeric_limits<int>::max(),
			"interiorRegionGap",
			"places the interior region outside the int coordinate range");

		const std::int64_t maximumEntitiesPerZone = static_cast<std::int64_t>(Config::MaximumPerZoneCount) +
													p_config.normalPoiPerZone + p_config.uncommonPoiPerZone + p_config.rarePoiPerZone;
		const std::int64_t maximumInteriorSpan =
			static_cast<std::int64_t>(p_config.zoneCount) * maximumEntitiesPerZone *
			p_config.interiorSlotBlocks;
		require(
			maximumInteriorSpan - worldWidth / 2 <= std::numeric_limits<int>::max(),
			"zoneCount/entity counts/interiorSlotBlocks",
			"can place the interior band outside the int coordinate range");
	}
}
