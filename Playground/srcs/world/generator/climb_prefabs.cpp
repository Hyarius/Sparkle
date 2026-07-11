#include "world/generator/climb_prefabs.hpp"

#include "core/deterministic_random.hpp"
#include "core/registry.hpp"
#include "core/weighted_pool.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/biome_definition.hpp"
#include "world/prefab_definition.hpp"

#include "structures/voxel/spk_prefab.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace pg
{
	namespace
	{
		using VoxelPool = WeightedPool<std::int32_t>;

		// Pre-mixed variants per biome flight pool: enough that neighboring staircases read
		// as distinct without flooding the registry. A single-voxel pool collapses to one.
		constexpr int kFlightVariants = 8;

		// FNV-1a over the generated id: a stable per-variant seed so the block mix is the
		// same on every run for a given data set (placement variety comes from the plan RNG
		// picking among variants).
		[[nodiscard]] spk::VoxelCell cellOf(std::int32_t p_id)
		{
			spk::VoxelCell cell;
			cell.id = p_id;
			return cell;
		}

		[[nodiscard]] spk::VoxelCell pickCell(const VoxelPool &p_pool, std::mt19937_64 &p_rng)
		{
			if (p_pool.size() == 1)
			{
				return cellOf(p_pool.front().value);
			}

			std::uniform_real_distribution<double> pick(0.0, p_pool.totalWeight());
			return cellOf(p_pool.pickTarget(pick(p_rng)));
		}

		// The staircase flight: a 3x3x3 box, three ascending rows of step blocks over a base
		// fill, its empty cells listed (via the grid) so stamping carves the air above the
		// ramp even against a cliff. Every block is drawn per cell from its pool for variety.
		[[nodiscard]] PrefabDefinition makeFlight(
			const VoxelPool &p_base,
			const VoxelPool &p_steps,
			std::mt19937_64 &p_rng)
		{
			spk::VoxelGrid grid(spk::Vector3Int{3, 3, 3});
			for (int x = 0; x < 3; ++x)
			{
				grid.cell(x, 0, 1) = pickCell(p_base, p_rng);
				grid.cell(x, 0, 2) = pickCell(p_base, p_rng);
				grid.cell(x, 1, 2) = pickCell(p_base, p_rng);
			}
			for (int x = 0; x < 3; ++x)
			{
				grid.cell(x, 0, 0) = pickCell(p_steps, p_rng);
				grid.cell(x, 1, 1) = pickCell(p_steps, p_rng);
				grid.cell(x, 2, 2) = pickCell(p_steps, p_rng);
			}
			return PrefabDefinition{.prefab = spk::Prefab(grid, spk::Vector3Int{0, 0, 0})};
		}

		// The platform pad: one solid 3x3 layer under the stamp, its cells drawn per block
		// from the base pool, with the authored clearance the world planner keeps free above.
		[[nodiscard]] PrefabDefinition makePlatform(const VoxelPool &p_base, std::mt19937_64 &p_rng)
		{
			spk::Prefab prefab;
			for (int z = 0; z <= 2; ++z)
			{
				for (int x = 0; x <= 2; ++x)
				{
					prefab.addVoxel({x, -1, z}, pickCell(p_base, p_rng));
				}
			}
			PrefabDefinition definition{.prefab = std::move(prefab)};
			definition.clearance = PrefabClearance{.min = {0, -1, 0}, .max = {2, 3, 2}};
			return definition;
		}

		// Generates the flight variants + platform for one climb category, registers them,
		// and records the resulting id pools. Both the base fill and the step blocks are
		// pools; a single-entry pool simply yields a uniform look.
		void buildCategory(
			Registry<PrefabDefinition> &p_prefabs,
			const std::string &p_lengthPrefix,
			const std::string &p_platformId,
			const VoxelPool &p_base,
			const VoxelPool &p_steps,
			std::vector<std::string> &p_outLengths,
			std::string &p_outPlatform)
		{
			const bool uniform = p_base.size() <= 1 && p_steps.size() <= 1;
			const int variants = uniform ? 1 : kFlightVariants;
			for (int index = 0; index < variants; ++index)
			{
				const std::string id = p_lengthPrefix + "#" + std::to_string(index);
				std::mt19937_64 rng(deterministic::fnv1a(id));
				p_prefabs.add(id, makeFlight(p_base, p_steps, rng));
				p_outLengths.push_back(id);
			}
			std::mt19937_64 platformRng(deterministic::fnv1a(p_platformId));
			p_prefabs.add(p_platformId, makePlatform(p_base, platformRng));
			p_outPlatform = p_platformId;
		}
	}

	void synthesizeClimbPrefabs(
		Registry<PrefabDefinition> &p_prefabs,
		Registry<BiomeDefinition> &p_biomes,
		const VoxelRegistry &p_voxels)
	{
		const auto numericIds = [&](const BiomePalette::VoxelPool &p_voxelsIds) {
			VoxelPool ids;
			ids.reserve(p_voxelsIds.size());
			for (const auto &voxel : p_voxelsIds)
			{
				ids.add(p_voxels.numericId(voxel.value), voxel.weight);
			}
			return ids;
		};

		for (const std::string &biomeId : p_biomes.ids())
		{
			BiomeDefinition &biome = p_biomes.getMutable(biomeId);
			if (!biome.worldgen.has_value())
			{
				continue; // interior biomes (caves, ...) build no staircases
			}
			BiomeWorldgenTraits &traits = *biome.worldgen;
			const BiomePalette &palette = biome.palette;
			// Road climbs pave with the biome road pool; wild climbs blend into the surface.
			if (!palette.road.empty() && !palette.stair.empty())
			{
				buildCategory(
					p_prefabs,
					biomeId + "-stair-length",
					biomeId + "-stair-platform",
					numericIds(palette.road),
					numericIds(palette.stair),
					traits.roadStairLengths,
					traits.roadStairPlatform);
			}
			if (!palette.surface.empty() && !palette.slope.empty())
			{
				buildCategory(
					p_prefabs,
					biomeId + "-slope-length",
					biomeId + "-slope-platform",
					numericIds(palette.surface),
					numericIds(palette.slope),
					traits.wildSlopeLengths,
					traits.wildSlopePlatform);
			}
		}
	}
}
