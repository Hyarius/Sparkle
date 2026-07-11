#include "world/generator/world_plan.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
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
			std::pair{"gymsPerZone", p_config.gymsPerZone},
			std::pair{"citiesPerZone", p_config.citiesPerZone},
			std::pair{"normalPoiPerZone", p_config.normalPoiPerZone},
			std::pair{"uncommonPoiPerZone", p_config.uncommonPoiPerZone},
			std::pair{"rarePoiPerZone", p_config.rarePoiPerZone},
			std::pair{"poiRoadConnections", p_config.poiRoadConnections},
			std::pair{"portsPerContinent", p_config.portsPerContinent},
			std::pair{"wildStairsPerZone", p_config.wildStairsPerZone}};
		for (const auto &[field, value] : integerCounts)
		{
			require(
				value >= 0 && value <= Config::MaximumPerZoneCount,
				field,
				"must be between 0 and MaximumPerZoneCount");
		}

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

		const std::array ratios = {
			std::pair{"distRatioCity", p_config.distRatioCity},
			std::pair{"distRatioGym", p_config.distRatioGym},
			std::pair{"distRatioPoi", p_config.distRatioPoi}};
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

		const std::int64_t maximumEntitiesPerZone =
			static_cast<std::int64_t>(p_config.gymsPerZone) + p_config.citiesPerZone +
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
