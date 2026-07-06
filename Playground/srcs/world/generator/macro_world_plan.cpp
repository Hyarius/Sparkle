#include "world/generator/macro_world_plan.hpp"

#include <bit>
#include <stdexcept>
#include <type_traits>

namespace
{
	void hashByte(std::uint64_t &p_hash, std::uint8_t p_value) noexcept
	{
		p_hash ^= p_value;
		p_hash *= 1099511628211ULL;
	}

	template <typename TType>
	void hashInteger(std::uint64_t &p_hash, TType p_value) noexcept
	{
		using Unsigned = std::make_unsigned_t<TType>;
		const Unsigned value = static_cast<Unsigned>(p_value);
		for (std::size_t i = 0; i < sizeof(value); ++i)
		{
			hashByte(p_hash, static_cast<std::uint8_t>(value >> (i * 8)));
		}
	}

	void hashString(std::uint64_t &p_hash, const std::string &p_value) noexcept
	{
		for (const unsigned char byte : p_value)
		{
			hashByte(p_hash, byte);
		}
		hashByte(p_hash, 0xff);
	}
}

namespace pg
{
	MacroWorldPlan::MacroWorldPlan(int p_width, int p_height, std::uint64_t p_runSeed) :
		width(p_width),
		height(p_height),
		runSeed(p_runSeed)
	{
		if (width <= 0 || height <= 0)
		{
			throw std::invalid_argument("macro world dimensions must be positive");
		}
		const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
		landMask.assign(count, 0);
		oceanMask.assign(count, 0);
		oceanDistances.assign(count, 0);
		macroRegionCells.assign(count, -1);
		landmassCells.assign(count, -1);
		heights.assign(count, 0.0f);
		riverMask.assign(count, 0);
		roadMask.assign(count, 0);
		featureMask.assign(count, 0);
		biomeCells.assign(count, -1);
	}

	bool MacroWorldPlan::contains(PlanCell p_cell) const noexcept
	{
		return p_cell.x >= 0 && p_cell.y >= 0 && p_cell.x < width && p_cell.y < height;
	}

	std::size_t MacroWorldPlan::index(PlanCell p_cell) const
	{
		if (!contains(p_cell))
		{
			throw std::out_of_range("macro world cell is outside the plan");
		}
		return static_cast<std::size_t>(p_cell.y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(p_cell.x);
	}

	PlanCell MacroWorldPlan::nodeCell(std::size_t p_node) const
	{
		if (p_node < cities.size())
		{
			return cities[p_node].cell;
		}
		p_node -= cities.size();
		if (p_node < settlements.size())
		{
			return settlements[p_node].cell;
		}
		throw std::out_of_range("macro world transport node does not exist");
	}

	PlanInfo MacroWorldPlan::queryCell(PlanCell p_cell) const
	{
		const std::size_t i = index(p_cell);
		PlanInfo result;
		result.land = landMask[i] != 0;
		result.ocean = oceanMask[i] != 0;
		result.oceanDistance = oceanDistances[i];
		result.macroRegion = macroRegionCells[i];
		result.landmass = landmassCells[i];
		result.height = heights[i];
		result.road = roadMask[i] != 0;
		result.river = riverMask[i] != 0;
		if (biomeCells[i] >= 0 && static_cast<std::size_t>(biomeCells[i]) < biomeIds.size())
		{
			result.biome = biomeIds[static_cast<std::size_t>(biomeCells[i])];
		}
		if (featureMask[i] != 0)
		{
			result.feature = static_cast<PlanFeatureType>(featureMask[i] - 1);
		}
		return result;
	}

	std::uint64_t MacroWorldPlan::hash() const noexcept
	{
		std::uint64_t result = 1469598103934665603ULL;
		hashInteger(result, width);
		hashInteger(result, height);
		hashInteger(result, runSeed);
		for (const std::uint8_t value : landMask)
		{
			hashByte(result, value);
		}
		for (const std::uint8_t value : oceanMask)
		{
			hashByte(result, value);
		}
		for (const std::uint16_t value : oceanDistances)
		{
			hashInteger(result, value);
		}
		for (const std::int32_t value : macroRegionCells)
		{
			hashInteger(result, value);
		}
		for (const std::int16_t value : landmassCells)
		{
			hashInteger(result, value);
		}
		for (const float value : heights)
		{
			hashInteger(result, std::bit_cast<std::uint32_t>(value));
		}
		for (const std::uint8_t value : riverMask)
		{
			hashByte(result, value);
		}
		for (const std::uint8_t value : roadMask)
		{
			hashByte(result, value);
		}
		for (const std::uint8_t value : featureMask)
		{
			hashByte(result, value);
		}
		for (const std::int16_t value : biomeCells)
		{
			hashInteger(result, value);
		}
		for (const std::string &value : biomeIds)
		{
			hashString(result, value);
		}
		for (const PlanCity &city : cities)
		{
			hashInteger(result, city.cell.x);
			hashInteger(result, city.cell.y);
			hashString(result, city.biome);
			hashInteger(result, std::bit_cast<std::uint32_t>(city.biomeWeight));
		}
		for (const PlanSettlement &settlement : settlements)
		{
			hashInteger(result, settlement.cell.x);
			hashInteger(result, settlement.cell.y);
			hashInteger(result, settlement.hubCity);
			hashString(result, settlement.biome);
		}
		for (const PlanTransportEdge &edge : edges)
		{
			hashInteger(result, edge.from);
			hashInteger(result, edge.to);
			hashByte(result, static_cast<std::uint8_t>(edge.classification));
		}
		for (const PlanRoad &road : roads)
		{
			hashInteger(result, road.edge);
			for (const PlanCell cell : road.cells)
			{
				hashInteger(result, cell.x);
				hashInteger(result, cell.y);
			}
		}
		for (const PlanFeature &feature : features)
		{
			hashByte(result, static_cast<std::uint8_t>(feature.type));
			hashInteger(result, feature.cell.x);
			hashInteger(result, feature.cell.y);
			hashInteger(result, feature.edge);
		}
		for (const PlanRiver &river : rivers)
		{
			hashByte(result, river.reachesSea ? 1 : 0);
			hashByte(result, river.endsInLakeBasin ? 1 : 0);
			for (const PlanCell cell : river.cells)
			{
				hashInteger(result, cell.x);
				hashInteger(result, cell.y);
			}
		}
		return result;
	}
}
