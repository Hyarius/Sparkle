#include "core/paths.hpp"
#include "core/registries.hpp"
#include "world/generator/plan_chunk_provider.hpp"
#include "world/prefab_placement_math.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <memory>

namespace
{
	const pg::Registries &registries()
	{
		static const std::unique_ptr<pg::Registries> value = [] {
			auto result = std::make_unique<pg::Registries>();
			result->loadAll(pg::resourceRoot() / "data");
			return result;
		}();
		return *value;
	}

	pg::WorldPlan reliefPlan()
	{
		pg::WorldPlan plan;
		plan.config.size = 16;
		plan.config.masterSeed = 0x7a11c0ffeeULL;
		plan.biomes.push_back({.id = "forest"});
		plan.zones.push_back({.id = 0, .biomeIndex = 0});
		plan.land = pg::PlanGrid<std::uint8_t>(16, 1);
		plan.zone = pg::PlanGrid<std::int16_t>(16, 0);
		plan.height = pg::PlanGrid<std::int8_t>(16, 2);
		plan.water = pg::PlanGrid<std::uint8_t>(16, 0);
		plan.lake = pg::PlanGrid<std::uint8_t>(16, 0);
		plan.road = pg::PlanGrid<std::uint8_t>(16, 1);
		plan.townPath = pg::PlanGrid<std::uint8_t>(16, 0);
		plan.bridge = pg::PlanGrid<std::uint8_t>(16, 0);
		return plan;
	}
}

TEST(PlanChunkProviderRelief, IsDeterministicBoundedAndUsesMaterialFamilySlabs)
{
	const pg::WorldPlan plan = reliefPlan();
	const pg::PlanChunkProvider first(registries(), plan);
	const pg::PlanChunkProvider second(registries(), plan);
	const int baseSurface = plan.surfaceY(2);
	const int offset = plan.worldOffset();

	// Cells on a common height plate may now share relief across their former
	// macro-cell boundary, allowing broad features instead of isolated 8x8 patches.
	bool reachesSharedPlateBoundary = false;
	for (int col = 1; col < plan.config.size && !reachesSharedPlateBoundary; ++col)
	{
		const int x = offset + col * plan.config.blocksPerCell;
		for (int z = offset; z < offset + plan.config.size * plan.config.blocksPerCell; ++z)
		{
			if (first.surfaceHeight(x - 1, z) != baseSurface || first.surfaceHeight(x, z) != baseSurface)
			{
				reachesSharedPlateBoundary = true;
				break;
			}
		}
	}
	EXPECT_TRUE(reachesSharedPlateBoundary);

	int slabCount = 0;
	int groundSlabCount = 0;
	int roadSlabCount = 0;
	const pg::VoxelRegistry &voxels = registries().voxels();
	for (int chunkZ = -4; chunkZ < 4; ++chunkZ)
	{
		for (int chunkX = -4; chunkX < 4; ++chunkX)
		{
			spk::VoxelChunk chunk({chunkX, 0, chunkZ}, voxels.renderRegistry());
			first.fill(chunk);
			for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
			{
				for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
				{
					const int worldX = chunk.worldOrigin().x + x;
					const int worldZ = chunk.worldOrigin().z + z;
					EXPECT_EQ(first.surfaceHeight(worldX, worldZ), second.surfaceHeight(worldX, worldZ));
					EXPECT_GE(first.surfaceHeight(worldX, worldZ), baseSurface - 1);
					EXPECT_LE(first.surfaceHeight(worldX, worldZ), baseSurface + 1);
					for (int y = 1; y < spk::VoxelChunk::Size.y; ++y)
					{
						const spk::VoxelCell &cell = chunk.cell({x, y, z});
						if (cell.isEmpty() || voxels.state(cell.id).name != "slab")
						{
							continue;
						}
						++slabCount;
						const pg::VoxelDefinition &material = voxels.definition(cell.id);
						const pg::VoxelDefinition &below = voxels.definition(chunk.cell({x, y - 1, z}).id);
						EXPECT_EQ(material.typeId, below.typeId);
						EXPECT_GE(y, baseSurface);
						EXPECT_LE(y, baseSurface + 1);
						if (material.id == "grass-block")
						{
							++groundSlabCount;
						}
						else if (material.id == "forest-road" || material.id == "sand-block")
						{
							++roadSlabCount;
						}
					}
				}
			}
		}
	}

	EXPECT_GT(slabCount, 0);
	EXPECT_LT(slabCount, plan.config.size * plan.config.size * plan.config.blocksPerCell * plan.config.blocksPerCell * 3 / 4);
	EXPECT_GT(groundSlabCount, 0);
	EXPECT_GT(roadSlabCount, 0);
}

TEST(PlanChunkProviderTownRelief, FadesPerlinFromTheConstructionFootprint)
{
	pg::WorldPlan plan = reliefPlan();
	plan.config.terrainVariationFeatureBlocks = 16.0;
	plan.config.terrainVariationOctaves = 1;
	plan.config.terrainVariationThreshold = 0.1;
	const int baseSurface = plan.surfaceY(2);
	const int center = plan.worldOffset() + 8 * plan.config.blocksPerCell;
	pg::PrefabPlacement placement{.prefabId = "town-house", .anchor = {center, baseSurface + 1, center}, .foundation = true};
	const pg::PrefabDefinition &prefab = registries().prefabs().get(placement.prefabId);
	const pg::ResolvedPlacementBox box = pg::resolvePlacement(prefab.prefab, placement);
	plan.placements.push_back(placement);
	pg::PlanTownRecord town;
	town.buildingPlacementIndices.push_back(0);
	plan.towns.push_back(std::move(town));

	const pg::PlanChunkProvider provider(registries(), plan);
	EXPECT_EQ(provider.surfaceHeight(box.worldMin.x - 1, box.worldMin.z + 2), baseSurface);

	bool hasFullStrengthRelief = false;
	for (int z = center - 40; z <= center + 40 && !hasFullStrengthRelief; ++z)
	{
		for (int x = center - 40; x <= center + 40; ++x)
		{
			if (std::abs(x - center) + std::abs(z - center) <= 16) continue;
			if (provider.surfaceHeight(x, z) != baseSurface)
			{
				hasFullStrengthRelief = true;
				break;
			}
		}
	}
	EXPECT_TRUE(hasFullStrengthRelief);
}
