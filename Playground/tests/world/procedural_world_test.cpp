#include <gtest/gtest.h>

#include "board/cell_source.hpp"
#include "core/registries.hpp"
#include "voxel/voxel_traversal.hpp"
#include "world/generator/procedural_chunk_provider.hpp"
#include "world/generator/procedural_world.hpp"
#include "world/voxel_world.hpp"

#include <cmath>

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

	pg::WorldgenParams defaultParams()
	{
		return pg::WorldgenParams::load(
			std::filesystem::path(PG_RESOURCE_DIR) / "data" / "worldgen" / "default.json");
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
}

TEST(ProceduralWorldTest, ManifestIsDeterministicSparseAndSeedSensitive)
{
	const pg::WorldgenParams params = defaultParams();
	const pg::ProceduralWorld first = pg::ProceduralWorld::generate(params, 20260705);
	const pg::ProceduralWorld repeated = pg::ProceduralWorld::generate(params, 20260705);
	const pg::ProceduralWorld other = pg::ProceduralWorld::generate(params, 20260706);
	EXPECT_EQ(first.hash(), repeated.hash());
	EXPECT_NE(first.hash(), other.hash());
	EXPECT_LT(first.manifestByteSize(), 256u * 1024u);
	EXPECT_EQ(first.cities().size(), static_cast<std::size_t>(params.cities.majorCount));
	EXPECT_EQ(first.roads().size(), first.cities().size() - 1 + static_cast<std::size_t>(params.transport.extraEdges));
	for (const pg::PlanCell cell : {pg::PlanCell{0, 0}, pg::PlanCell{511, 511}, pg::PlanCell{1023, 1023}})
	{
		const pg::ProceduralTerrainSample a = first.sampleTerrain(cell);
		const pg::ProceduralTerrainSample b = repeated.sampleTerrain(cell);
		EXPECT_EQ(a.land, b.land);
		EXPECT_EQ(a.height, b.height);
		EXPECT_EQ(a.biome, b.biome);
	}
}

TEST(ProceduralWorldTest, LandRoadsAreCardinalGradeLimitedAndQueryable)
{
	const pg::ProceduralWorld world = pg::ProceduralWorld::generate(defaultParams(), 20260705);
	for (const pg::ProceduralRoad &road : world.roads())
	{
		if (road.classification == pg::TransportClass::Sea)
		{
			continue;
		}
		ASSERT_FALSE(road.cells.empty());
		for (std::size_t i = 0; i < road.cells.size(); ++i)
		{
			const pg::ProceduralColumnSample sample = world.sample(road.cells[i].cell);
			EXPECT_TRUE(sample.land);
			EXPECT_TRUE(sample.road);
			if (i == 0)
			{
				continue;
			}
			EXPECT_EQ(
				std::abs(road.cells[i].cell.x - road.cells[i - 1].cell.x) +
					std::abs(road.cells[i].cell.y - road.cells[i - 1].cell.y),
				1);
			EXPECT_LE(std::abs(road.cells[i].height - road.cells[i - 1].height), 1);
		}
	}
}

TEST(ProceduralWorldTest, EveryContinentHasACoastalPortAndSeaRoutesUsePorts)
{
	const pg::ProceduralWorld world = pg::ProceduralWorld::generate(defaultParams(), 20260705);
	for (std::size_t continent = 0; continent < world.continents().size(); ++continent)
	{
		const auto port = std::ranges::find_if(world.cities(), [&](const pg::ProceduralCity &p_city) {
			return p_city.port && p_city.continent == static_cast<int>(continent);
		});
		ASSERT_NE(port, world.cities().end());
		const pg::ProceduralTerrainSample sample = world.sampleTerrain(port->cell);
		EXPECT_TRUE(sample.land);
		EXPECT_EQ(sample.biome, "coast");
		EXPECT_EQ(sample.continent, static_cast<int>(continent));
	}
	for (const pg::ProceduralRoad &road : world.roads())
	{
		if (road.classification != pg::TransportClass::Sea)
		{
			continue;
		}
		ASSERT_LT(road.fromCity, world.cities().size());
		ASSERT_LT(road.toCity, world.cities().size());
		EXPECT_TRUE(world.cities()[road.fromCity].port);
		EXPECT_TRUE(world.cities()[road.toCity].port);
		EXPECT_NE(world.cities()[road.fromCity].continent, world.cities()[road.toCity].continent);
	}
}

TEST(ProceduralWorldTest, RequestedChunkRegeneratesWithoutAWorldRaster)
{
	const pg::ProceduralWorld world = pg::ProceduralWorld::generate(defaultParams(), 20260705);
	const pg::ProceduralChunkProvider provider(
		world, registries().biomes(), registries().prefabs(), registries().voxels(), 20260705);
	pg::VoxelWorld first(registries().voxels());
	pg::VoxelWorld second(registries().voxels());
	const pg::ChunkCoordinates coordinates = pg::ChunkCoordinates::fromWorldCell(provider.spawnCell());
	EXPECT_EQ(chunkHash(first.loadChunk(coordinates, provider)), chunkHash(second.loadChunk(coordinates, provider)));
}

TEST(ProceduralWorldTest, LazyWorldProducesAStandableSpawn)
{
	const pg::ProceduralWorld world = pg::ProceduralWorld::generate(defaultParams(), 20260705);
	const pg::ProceduralChunkProvider provider(
		world, registries().biomes(), registries().prefabs(), registries().voxels(), 20260705);
	pg::VoxelWorld voxelWorld(registries().voxels());
	const spk::Vector3Int spawn = provider.spawnCell();
	(void)voxelWorld.loadChunk(pg::ChunkCoordinates::fromWorldCell(spawn), provider);
	(void)voxelWorld.loadChunk(
		pg::ChunkCoordinates::fromWorldCell(spawn + spk::Vector3Int{0, 2, 0}), provider);
	EXPECT_TRUE(pg::isStandable(pg::WorldCellSource(voxelWorld), spawn));
}
