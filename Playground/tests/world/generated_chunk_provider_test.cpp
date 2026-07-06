#include <gtest/gtest.h>

#include "board/cell_source.hpp"
#include "core/registries.hpp"
#include "voxel/voxel_traversal.hpp"
#include "world/generator/generated_chunk_provider.hpp"
#include "world/generator/macro_world_generator.hpp"
#include "world/voxel_world.hpp"

#include <cmath>
#include <map>
#include <set>

namespace
{
	const pg::Registries &registries()
	{
		static pg::Registries value;
		static const bool loaded = [] {
			value.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return true;
		}();
		(void)loaded;
		return value;
	}

	pg::WorldgenParams providerParams()
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
		params.transport.tunnelTriggerCost = 100000.0f;
		return params;
	}

	std::uint64_t chunkHash(const pg::Chunk &p_chunk)
	{
		std::uint64_t result = 1469598103934665603ULL;
		for (const pg::VoxelCell &cell : p_chunk.grid().cells())
		{
			result ^= static_cast<std::uint32_t>(cell.id);
			result *= 1099511628211ULL;
			result ^= static_cast<std::uint8_t>(cell.orientation);
			result *= 1099511628211ULL;
		}
		return result;
	}

	pg::VoxelOrientation direction(pg::PlanCell p_from, pg::PlanCell p_to)
	{
		if (p_to.x > p_from.x)
		{
			return pg::VoxelOrientation::PositiveX;
		}
		if (p_to.x < p_from.x)
		{
			return pg::VoxelOrientation::NegativeX;
		}
		if (p_to.y > p_from.y)
		{
			return pg::VoxelOrientation::PositiveZ;
		}
		return pg::VoxelOrientation::NegativeZ;
	}

	pg::VoxelOrientation opposite(pg::VoxelOrientation p_direction)
	{
		switch (p_direction)
		{
		case pg::VoxelOrientation::PositiveX:
			return pg::VoxelOrientation::NegativeX;
		case pg::VoxelOrientation::NegativeX:
			return pg::VoxelOrientation::PositiveX;
		case pg::VoxelOrientation::PositiveZ:
			return pg::VoxelOrientation::NegativeZ;
		case pg::VoxelOrientation::NegativeZ:
			return pg::VoxelOrientation::PositiveZ;
		}
		return pg::VoxelOrientation::PositiveZ;
	}
}

TEST(GeneratedChunkProviderTest, RecreatesIdenticalChunkContent)
{
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(providerParams(), 301);
	const pg::GeneratedChunkProvider provider(plan, registries().biomes(), registries().prefabs(), registries().voxels(), 301);
	pg::VoxelWorld first(registries().voxels());
	pg::VoxelWorld second(registries().voxels());
	const pg::ChunkCoordinates coordinates = pg::ChunkCoordinates::fromWorldCell(provider.spawnCell());
	EXPECT_EQ(chunkHash(first.loadChunk(coordinates, provider)), chunkHash(second.loadChunk(coordinates, provider)));
}

TEST(GeneratedChunkProviderTest, DefaultConfigurationProducesAStandableSpawn)
{
	pg::WorldgenParams params = pg::WorldgenParams::load(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "worldgen" / "default.json");
	params.worldSize = {512, 512};
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(params, 1);
	const pg::GeneratedChunkProvider provider(
		plan, registries().biomes(), registries().prefabs(), registries().voxels(), 1);
	pg::VoxelWorld world(registries().voxels());
	const spk::Vector3Int spawn = provider.spawnCell();
	(void)world.loadChunk(pg::ChunkCoordinates::fromWorldCell(spawn), provider);
	(void)world.loadChunk(pg::ChunkCoordinates::fromWorldCell(spawn + spk::Vector3Int{0, 2, 0}), provider);
	EXPECT_TRUE(pg::isStandable(pg::WorldCellSource(world), spawn));
}

TEST(GeneratedChunkProviderTest, RealizedLandRoadsAreCardinalAndWalkableAcrossChunkBorders)
{
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(providerParams(), 302);
	const pg::GeneratedChunkProvider provider(plan, registries().biomes(), registries().prefabs(), registries().voxels(), 302);
	pg::VoxelWorld world(registries().voxels());
	std::set<pg::ChunkCoordinates> required;
	for (const pg::PlanRoad &road : plan.roads)
	{
		if (plan.edges[road.edge].classification == pg::TransportClass::Sea)
		{
			continue;
		}
		for (const pg::PlanCell cell : road.cells)
		{
			required.insert(pg::ChunkCoordinates::fromWorldCell({cell.x, provider.surfaceHeight(cell), cell.y}));
			required.insert(pg::ChunkCoordinates::fromWorldCell({cell.x, provider.surfaceHeight(cell) + 2, cell.y}));
		}
	}
	for (const pg::ChunkCoordinates coordinates : required)
	{
		(void)world.loadChunk(coordinates, provider);
	}

	const pg::WorldCellSource source(world);
	for (const pg::PlanRoad &road : plan.roads)
	{
		if (plan.edges[road.edge].classification == pg::TransportClass::Sea)
		{
			continue;
		}
		for (std::size_t i = 1; i < road.cells.size(); ++i)
		{
			const pg::PlanCell a = road.cells[i - 1];
			const pg::PlanCell b = road.cells[i];
			SCOPED_TRACE("road " + std::to_string(road.edge) + " at " + std::to_string(a.x) + "," + std::to_string(a.y) + " -> " + std::to_string(b.x) + "," + std::to_string(b.y));
			EXPECT_EQ(std::abs(a.x - b.x) + std::abs(a.y - b.y), 1);
			EXPECT_LE(std::abs(provider.surfaceHeight(a) - provider.surfaceHeight(b)), 1);
			const spk::Vector3Int aWorld{a.x, provider.surfaceHeight(a), a.y};
			const spk::Vector3Int bWorld{b.x, provider.surfaceHeight(b), b.y};
			ASSERT_TRUE(pg::isStandable(source, aWorld)) << "voxel " << world.cell(aWorld).id;
			ASSERT_TRUE(pg::isStandable(source, bWorld)) << "voxel " << world.cell(bWorld).id
														 << " above " << world.cell(bWorld + spk::Vector3Int{0, 1, 0}).id
														 << " above2 " << world.cell(bWorld + spk::Vector3Int{0, 2, 0}).id;
			const pg::VoxelOrientation travel = direction(a, b);
			EXPECT_LE(std::abs(pg::walkHeightAtEdge(source, aWorld, travel) - pg::walkHeightAtEdge(source, bWorld, opposite(travel))), 0.5001f);
		}
	}
}

TEST(GeneratedChunkProviderTest, FloraScatterIsDeterministicAndWithinBiomeDensityRange)
{
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(providerParams(), 303);
	const pg::GeneratedChunkProvider first(plan, registries().biomes(), registries().prefabs(), registries().voxels(), 303);
	const pg::GeneratedChunkProvider repeated(plan, registries().biomes(), registries().prefabs(), registries().voxels(), 303);
	std::map<std::string, std::pair<int, int>> counts;
	for (int y = 0; y < plan.height; ++y)
	{
		for (int x = 0; x < plan.width; ++x)
		{
			const pg::PlanCell cell{x, y};
			const pg::PlanInfo info = plan.queryCell(cell);
			if (!info.land || info.road || info.river)
			{
				continue;
			}
			++counts[info.biome].second;
			const bool placed = first.shouldPlaceFlora(cell);
			EXPECT_EQ(placed, repeated.shouldPlaceFlora(cell));
			if (placed)
			{
				++counts[info.biome].first;
			}
		}
	}
	for (const auto &[biome, count] : counts)
	{
		(void)biome;
		if (count.second < 100)
		{
			continue;
		}
		const float density = static_cast<float>(count.first) / static_cast<float>(count.second);
		EXPECT_GT(density, 0.005f);
		EXPECT_LT(density, 0.11f);
	}
}

TEST(GeneratedChunkProviderTest, PrefabStampsAreAnchoredOnPlannedLand)
{
	const pg::MacroWorldPlan plan = pg::MacroWorldGenerator::generate(providerParams(), 304);
	const pg::GeneratedChunkProvider provider(plan, registries().biomes(), registries().prefabs(), registries().voxels(), 304);
	ASSERT_FALSE(provider.stamps().empty());
	pg::VoxelWorld world(registries().voxels());
	for (const pg::GeneratedStamp &stamp : provider.stamps())
	{
		EXPECT_TRUE(plan.queryCell(stamp.planCell).land);
		const pg::ChunkCoordinates coordinates = pg::ChunkCoordinates::fromWorldCell(stamp.origin);
		(void)world.loadChunk(coordinates, provider);
		EXPECT_FALSE(world.cell(stamp.origin).isEmpty());
	}
}
