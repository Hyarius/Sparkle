#include <gtest/gtest.h>

#include "world/generator/macro_world_generator.hpp"
#include "world/generator/worldgen_params.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <queue>
#include <ranges>

namespace
{
	pg::WorldgenParams testParams()
	{
		pg::WorldgenParams params;
		params.worldSize = {128, 128};
		params.landmass.regionCount = 90;
		params.landmass.landmassCount = {1, 2};
		params.landmass.landRatio = 0.48f;
		params.landmass.borderOceanWidth = 4;
		params.landmass.noiseFrequency = 0.016f;
		params.landmass.detailFrequency = 0.05f;
		params.height.baseFrequency = 0.025f;
		params.height.mountainFrequency = 0.06f;
		params.height.maxHeight = 24;
		params.cities.majorCount = 4;
		params.cities.minSpacing = 20.0f;
		params.cities.satellitesPerCity = {1, 1};
		params.cities.satelliteRadius = 18.0f;
		params.biomes = {"forest", "desert", "tundra", "swamp"};
		params.transport.extraEdges = 1;
		params.transport.tunnelTriggerCost = 120.0f;
		return params;
	}

	bool allNodesReachable(const pg::MacroWorldPlan &p_plan)
	{
		const std::size_t count = p_plan.cities.size() + p_plan.settlements.size();
		std::vector<bool> reached(count, false);
		std::queue<std::size_t> pending;
		reached[0] = true;
		pending.push(0);
		while (!pending.empty())
		{
			const std::size_t node = pending.front();
			pending.pop();
			for (const pg::PlanTransportEdge &edge : p_plan.edges)
			{
				std::size_t next = count;
				if (edge.from == node)
				{
					next = edge.to;
				}
				else if (edge.to == node)
				{
					next = edge.from;
				}
				if (next < count && !reached[next])
				{
					reached[next] = true;
					pending.push(next);
				}
			}
		}
		return std::ranges::all_of(reached, [](bool p_value) {
			return p_value;
		});
	}
}

TEST(MacroWorldGeneratorTest, IsDeterministicAndSeedSensitive)
{
	const pg::WorldgenParams params = testParams();
	const pg::MacroWorldPlan first = pg::MacroWorldGenerator::generate(params, 123456);
	const pg::MacroWorldPlan repeated = pg::MacroWorldGenerator::generate(params, 123456);
	const pg::MacroWorldPlan other = pg::MacroWorldGenerator::generate(params, 654321);
	EXPECT_EQ(first.hash(), repeated.hash());
	EXPECT_NE(first.hash(), other.hash());
}

TEST(MacroWorldGeneratorTest, LoadsTheStrictDefaultParameterFile)
{
	const pg::WorldgenParams params = pg::WorldgenParams::load(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "worldgen" / "default.json");
	EXPECT_EQ(params.version, 1);
	EXPECT_EQ(params.worldSize, (std::array<int, 2>{1024, 1024}));
	EXPECT_FALSE(params.biomes.empty());
}

TEST(MacroWorldGeneratorTest, SatisfiesMacroPlanInvariants)
{
	const pg::WorldgenParams params = testParams();
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(params, 98765);
	ASSERT_EQ(plan.cities.size(), static_cast<std::size_t>(params.cities.majorCount));
	for (std::size_t a = 0; a < plan.cities.size(); ++a)
	{
		EXPECT_TRUE(plan.queryCell(plan.cities[a].cell).land);
		for (std::size_t b = a + 1; b < plan.cities.size(); ++b)
		{
			const float dx = static_cast<float>(plan.cities[a].cell.x - plan.cities[b].cell.x);
			const float dy = static_cast<float>(plan.cities[a].cell.y - plan.cities[b].cell.y);
			EXPECT_GE(std::sqrt(dx * dx + dy * dy), params.cities.minSpacing - 1.5f);
		}
	}
	EXPECT_EQ(plan.cities.size(), static_cast<std::size_t>(params.cities.majorCount));
	EXPECT_EQ(plan.biomeIds.size(), plan.cities.size());
	EXPECT_TRUE(allNodesReachable(plan));
	EXPECT_EQ(plan.roads.size(), plan.edges.size());
	for (const pg::PlanRiver &river : plan.rivers)
	{
		EXPECT_TRUE(river.reachesSea || river.endsInLakeBasin);
	}
	for (const pg::PlanTransportEdge &edge : plan.edges)
	{
		EXPECT_LE(static_cast<int>(edge.classification), static_cast<int>(pg::TransportClass::Tunnel));
	}
	for (const pg::PlanRoad &road : plan.roads)
	{
		const bool crossesWater = std::ranges::any_of(road.cells, [&](pg::PlanCell p_cell) {
			return !plan.queryCell(p_cell).land;
		});
		if (!crossesWater)
		{
			continue;
		}
		EXPECT_EQ(plan.edges[road.edge].classification, pg::TransportClass::Sea);
		EXPECT_TRUE(std::ranges::any_of(plan.features, [&](const pg::PlanFeature &p_feature) {
			return p_feature.edge == road.edge && p_feature.type == pg::PlanFeatureType::Port;
		}));
	}
}

TEST(MacroWorldGeneratorTest, ExtraTransportEdgesDoNotChangeCities)
{
	pg::WorldgenParams fewEdges = testParams();
	pg::WorldgenParams manyEdges = fewEdges;
	manyEdges.transport.extraEdges = 4;
	const pg::MacroWorldPlan first = pg::MacroWorldGenerator::generate(fewEdges, 42);
	const pg::MacroWorldPlan second = pg::MacroWorldGenerator::generate(manyEdges, 42);
	ASSERT_EQ(first.cities.size(), second.cities.size());
	for (std::size_t i = 0; i < first.cities.size(); ++i)
	{
		EXPECT_EQ(first.cities[i].cell, second.cities[i].cell);
		EXPECT_EQ(first.cities[i].biome, second.cities[i].biome);
	}
}

TEST(MacroWorldGeneratorTest, BuildsMajorLandmassesAndPlacesCoastRelativeToOcean)
{
	pg::WorldgenParams params = pg::WorldgenParams::load(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "worldgen" / "default.json");
	params.worldSize = {512, 512};
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(params, 20260704);
	std::vector<bool> reached(plan.landMask.size(), false);
	std::size_t componentCount = 0;
	for (std::size_t start = 0; start < plan.landMask.size(); ++start)
	{
		if (!plan.landMask[start] || reached[start])
		{
			continue;
		}
		++componentCount;
		std::queue<std::size_t> pending;
		reached[start] = true;
		pending.push(start);
		const std::int16_t landmass = plan.landmassCells[start];
		ASSERT_GE(landmass, 0);
		while (!pending.empty())
		{
			const std::size_t index = pending.front();
			pending.pop();
			EXPECT_EQ(plan.landmassCells[index], landmass);
			const pg::PlanCell cell{
				static_cast<int>(index % static_cast<std::size_t>(plan.width)),
				static_cast<int>(index / static_cast<std::size_t>(plan.width))};
			for (const pg::PlanCell offset : std::array<pg::PlanCell, 4>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}})
			{
				const pg::PlanCell neighbour{cell.x + offset.x, cell.y + offset.y};
				if (!plan.contains(neighbour))
				{
					continue;
				}
				const std::size_t neighbourIndex = plan.index(neighbour);
				if (!plan.landMask[neighbourIndex] || reached[neighbourIndex])
				{
					continue;
				}
				reached[neighbourIndex] = true;
				pending.push(neighbourIndex);
			}
		}
	}
	EXPECT_GE(componentCount, static_cast<std::size_t>(params.landmass.landmassCount[0]));
	EXPECT_LE(componentCount, static_cast<std::size_t>(params.landmass.landmassCount[1]));
	const double landRatio = static_cast<double>(std::ranges::count(plan.landMask, static_cast<std::uint8_t>(1))) /
							 static_cast<double>(plan.landMask.size());
	EXPECT_NEAR(landRatio, params.landmass.landRatio, 0.08);
	for (int x = 0; x < plan.width; ++x)
	{
		EXPECT_TRUE(plan.queryCell({x, 0}).ocean);
		EXPECT_TRUE(plan.queryCell({x, plan.height - 1}).ocean);
	}
	for (int y = 0; y < plan.height; ++y)
	{
		EXPECT_TRUE(plan.queryCell({0, y}).ocean);
		EXPECT_TRUE(plan.queryCell({plan.width - 1, y}).ocean);
	}

	double coastDistanceSum = 0.0;
	double landDistanceSum = 0.0;
	std::size_t coastCells = 0;
	std::size_t landCells = 0;
	for (int y = 0; y < plan.height; ++y)
	{
		for (int x = 0; x < plan.width; ++x)
		{
			const pg::PlanInfo info = plan.queryCell({x, y});
			if (!info.land)
			{
				continue;
			}
			landDistanceSum += info.oceanDistance;
			++landCells;
			if (info.biome == "coast")
			{
				coastDistanceSum += info.oceanDistance;
				++coastCells;
			}
		}
	}
	ASSERT_GT(coastCells, 0u);
	EXPECT_LT(coastDistanceSum / static_cast<double>(coastCells), landDistanceSum / static_cast<double>(landCells));
}
