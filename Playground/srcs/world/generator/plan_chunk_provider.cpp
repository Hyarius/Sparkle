#include "world/generator/plan_chunk_provider.hpp"

#include "core/deterministic_random.hpp"
#include "core/registries.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/prefab_definition.hpp"
#include "world/prefab_placement_math.hpp"
#include "world/generator/path_surface.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pg
{
	namespace
	{
		using VoxelPool = WeightedPool<spk::VoxelRuntimeId>;

		constexpr std::uint64_t kSurfaceSalt = 0x6eed0e9da4d94a4fULL;
		constexpr std::uint64_t kSubsurfaceSalt = 0x58f0f1a32db7c2bdULL;
		constexpr std::uint64_t kDeepSalt = 0xf4d3b9a05f19e8b7ULL;
		constexpr std::uint64_t kRoadSalt = 0x314adbe6734a92d1ULL;

		// Deterministic per-column pick from a weighted pool. The final avalanche matters:
		// without it, two variants can collapse into visible parity/checker patterns.
		[[nodiscard]] spk::VoxelRuntimeId pickVoxel(
			const VoxelPool &p_pool,
			int p_worldX,
			int p_worldZ,
			std::uint64_t p_seed,
			std::uint64_t p_salt)
		{
			if (p_pool.size() <= 1)
			{
				return p_pool.front().value;
			}
			std::uint64_t hash = static_cast<std::uint64_t>(static_cast<std::uint32_t>(p_worldX)) << 32U;
			hash |= static_cast<std::uint32_t>(p_worldZ);
			hash ^= p_seed + p_salt + 0x9e3779b97f4a7c15ULL;
			hash = deterministic::avalanche(hash);
			return p_pool.pick(deterministic::unitInterval(hash));
		}
	}

	PlanChunkProvider::PlanChunkProvider(const Registries &p_registries, const WorldPlan &p_plan) :
		_plan(p_plan)
	{
		const VoxelRegistry &voxels = p_registries.voxels();
		_road = voxels.runtimeId("road-block");
		_water = voxels.runtimeId("water");
		_sand = voxels.runtimeId("sand-block");
		_stone = voxels.runtimeId("stone-block");
		_fallbackBlocks = {
			.surface = {{_stone, 1.0}},
			.subsurface = {{_stone, 1.0}},
			.deep = {{_stone, 1.0}},
			.road = {{_road, 1.0}}};

		_biomeBlocks.reserve(_plan.biomes.size());
		for (const PlanBiome &biome : _plan.biomes)
		{
			const BiomeDefinition &definition = p_registries.biomes().get(biome.id);
			const auto numericPool = [&voxels](const BiomePalette::VoxelPool &p_pool) {
				VoxelPool result;
				result.reserve(p_pool.size());
				for (const auto &entry : p_pool)
				{
					result.add(voxels.runtimeId(entry.value.id, entry.value.state), entry.weight);
				}
				return result;
			};
			VoxelPool road = numericPool(definition.palette.road);
			if (road.empty())
			{
				road.add(_road); // biomes that declare no road fall back to the shared block
			}
			_biomeBlocks.push_back(
				{.surface = numericPool(definition.palette.surface),
				 .subsurface = numericPool(definition.palette.subsurface),
				 .deep = numericPool(definition.palette.deep),
					 .road = std::move(road)});
		}
		for (const PlanTownRecord &town : _plan.towns)
		{
			for (const PlanPavedColumn &column : town.urbanRoadSurface)
			{
				_urbanRoadSurfaceY.emplace(std::pair{column.worldX, column.worldZ}, column.surfaceY);
			}
		}

		for (const PrefabPlacement &placement : _plan.placements)
		{
			const PrefabDefinition *prefab = p_registries.prefabs().tryGet(placement.prefabId);
			if (prefab == nullptr)
			{
				std::cerr << "world plan references unknown prefab '" << placement.prefabId << "', skipping"
						  << std::endl;
				continue;
			}
			// Shared with the world planner (prefab_placement_math.hpp): the plan's
			// claimed zones and stair footprints reason about exactly this box.
			const ResolvedPlacementBox box = resolvePlacement(prefab->prefab, placement);
			int realizationMinY = box.worldMin.y;
			if (placement.foundation)
			{
				for (int dz = 0; dz < box.extents.z; ++dz)
				{
					for (int dx = 0; dx < box.extents.x; ++dx)
					{
						const Column column = _column(box.worldMin.x + dx, box.worldMin.z + dz);
						realizationMinY = std::min(realizationMinY, column.groundTop + 1);
					}
				}
			}
			_placements.push_back(
				{.definition = prefab,
				 .worldMin = box.worldMin,
				 .rotatedSize = box.extents,
				 .destination = box.destination,
				 .realizationMinY = realizationMinY,
				 .orientation = placement.orientation,
				 .foundation = placement.foundation});
		}
	}

	PlanChunkProvider::Column PlanChunkProvider::_column(int p_worldX, int p_worldZ) const
	{
		const WorldGenConfig &cfg = _plan.config;
		const int blocks = cfg.blocksPerCell;
		const int offset = _plan.worldOffset();
		const int col = _plan.cellIndexFromWorld(p_worldX);
		const int row = _plan.cellIndexFromWorld(p_worldZ);

		const auto isOceanCell = [&](int p_row, int p_col) {
			return !_plan.land.contains(p_row, p_col) || _plan.land.at(p_row, p_col) == 0;
		};

		Column column;
		// The interior band east of the world stays void: composed rooms bring their
		// own floors and the emptiness reads as "elsewhere" from inside a house.
		if (_plan.isInteriorColumn(p_worldX))
		{
			return column;
		}
		if (isOceanCell(row, col))
		{
			const PlanTownRecord *dockTown=nullptr;for(const PlanTownRecord &town:_plan.towns)if(std::ranges::find(town.dockCells,std::pair{row,col})!=town.dockCells.end()){dockTown=&town;break;}
			if(dockTown!=nullptr&&dockTown->boardingEndpoint){column.groundTop=dockTown->boardingEndpoint->y-1;column.surfaceId=pickVoxel(_fallbackBlocks.road,p_worldX,p_worldZ,cfg.masterSeed,kRoadSalt);column.subsurfaceId=_sand;column.deepId=_stone;column.waterY=-1;return column;}
			column.groundTop = 1;
			column.surfaceId = _sand;
			column.subsurfaceId = _sand;
			column.deepId = _stone;
			column.waterY = 2;
			return column;
		}

		const int level = _plan.height.at(row, col);
		const int surface = _plan.surfaceY(level);
		const int zone = _plan.zone.at(row, col);
		const BiomeBlocks &blocksOfBiome =
			zone >= 0 ? _biomeBlocks[_plan.zones[zone].biomeIndex] : _fallbackBlocks;
		column.groundTop = surface;
		column.surfaceId = pickVoxel(blocksOfBiome.surface, p_worldX, p_worldZ, cfg.masterSeed, kSurfaceSalt);
		column.subsurfaceId = pickVoxel(blocksOfBiome.subsurface, p_worldX, p_worldZ, cfg.masterSeed, kSubsurfaceSalt);
		column.deepId = pickVoxel(blocksOfBiome.deep, p_worldX, p_worldZ, cfg.masterSeed, kDeepSalt);

		// Local position inside the plan cell + membership in the 3-wide central strip.
		const int localX = p_worldX - (offset + col * blocks);
		const int localZ = p_worldZ - (offset + row * blocks);
		// A strip cross: the center pad plus one arm toward each connected neighbor.
		const auto inCross = [&](bool p_north, bool p_south, bool p_west, bool p_east) {
			return isCenteredPathSurface(blocks, localX, localZ, p_north, p_south, p_west, p_east);
		};

		const bool isLake = _plan.lake.at(row, col) != 0;
		const bool isRiver = !isLake && _plan.water.at(row, col) != 0;
		if (isLake)
		{
			column.groundTop = surface - 2;
			column.surfaceId = column.deepId;
			column.subsurfaceId = column.deepId;
			column.waterY = surface - 1;
		}
		else if (isRiver)
		{
			const auto wet = [&](int p_row, int p_col) {
				return isOceanCell(p_row, p_col) || _plan.water.at(p_row, p_col) != 0;
			};
			if (inCross(wet(row - 1, col), wet(row + 1, col), wet(row, col - 1), wet(row, col + 1)))
			{
				column.groundTop = surface - 2;
				column.surfaceId = column.deepId;
				column.subsurfaceId = column.deepId;
				column.waterY = surface - 1;
			}
		}

		const bool exactUrbanRoad = _urbanRoadSurfaceY.contains({p_worldX, p_worldZ});
		if (_plan.road.at(row, col) != 0 || exactUrbanRoad)
		{
			const auto paved = [&](int p_row, int p_col) {
				return _plan.road.contains(p_row, p_col) && _plan.road.at(p_row, p_col) != 0;
			};
			if (exactUrbanRoad || inCross(paved(row - 1, col), paved(row + 1, col), paved(row, col - 1), paved(row, col + 1)))
			{
				// On bridges the deck sits at ground level on a solid pier; the water
				// column underneath is filled, its neighbors keep flowing around it.
				column.groundTop = surface;
				column.surfaceId = pickVoxel(blocksOfBiome.road, p_worldX, p_worldZ, cfg.masterSeed, kRoadSalt);
				column.subsurfaceId = column.deepId;
				column.waterY = -1;
			}
		}

		// Stairway approach bands: the road-width path beside a composed staircase,
		// paved so the road visibly turns at the cliff and reaches the bottom platform.
		// The generator validated these columns as flat, dry land.
		for (const PlanStairway &stairway : _plan.stairways)
		{
			if (!stairway.pavedApproach.has_value())
			{
				continue;
			}
			const PlanStairRect &rect = *stairway.pavedApproach;
			if (p_worldX >= rect.minX && p_worldX <= rect.maxX && p_worldZ >= rect.minZ && p_worldZ <= rect.maxZ)
			{
				column.surfaceId = pickVoxel(blocksOfBiome.road, p_worldX, p_worldZ, cfg.masterSeed, kRoadSalt);
				break;
			}
		}
		return column;
	}

	void PlanChunkProvider::fill(spk::VoxelChunk &p_chunk) const
	{
		const spk::Vector3Int origin = p_chunk.worldOrigin();
		if (origin.y + spk::VoxelChunk::Size.y <= 0)
		{
			return; // below the world: keep it empty
		}

		p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
			for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
			{
				for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
				{
					const Column column = _column(origin.x + x, origin.z + z);
					for (int y = 0; y < spk::VoxelChunk::Size.y; ++y)
					{
						const int worldY = origin.y + y;
						spk::VoxelCell value;
						if (worldY >= 0 && worldY <= column.groundTop)
						{
							const int depth = column.groundTop - worldY;
							if (depth == 0)
							{
								value.id = column.surfaceId;
							}
							else if (depth <= column.subsurfaceDepth)
							{
								value.id = column.subsurfaceId;
							}
							else
							{
								value.id = column.deepId;
							}
						}
						else if (worldY == column.waterY)
						{
							value.id = _water;
						}
						else
						{
							continue;
						}
						(void)p_editor.setCell(x, y, z, value);
					}
				}
			}
		});

		const spk::Vector3Int chunkMin = origin;
		const spk::Vector3Int chunkMax = origin + spk::VoxelChunk::Size;
		for (const ResolvedPlacement &placement : _placements)
		{
			const bool intersects = placement.worldMin.x < chunkMax.x &&
									placement.worldMin.x + placement.rotatedSize.x > chunkMin.x &&
									placement.worldMin.z < chunkMax.z &&
									placement.worldMin.z + placement.rotatedSize.z > chunkMin.z &&
									placement.realizationMinY < chunkMax.y &&
									placement.worldMin.y + placement.rotatedSize.y > chunkMin.y;
			if (intersects)
			{
				_stamp(p_chunk, placement);
			}
		}
	}

	void PlanChunkProvider::_stamp(spk::VoxelChunk &p_chunk, const ResolvedPlacement &p_placement) const
	{
		const spk::Vector3Int origin = p_chunk.worldOrigin();
		p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
			// Foundation: solid pillar from below the box down to the terrain, so ramps and
			// buildings never float over a carved channel or a lower neighboring cell.
			if (p_placement.foundation)
			{
				for (int dz = 0; dz < p_placement.rotatedSize.z; ++dz)
				{
					for (int dx = 0; dx < p_placement.rotatedSize.x; ++dx)
					{
						const int worldX = p_placement.worldMin.x + dx;
						const int worldZ = p_placement.worldMin.z + dz;
						const Column column = _column(worldX, worldZ);
						for (int worldY = column.groundTop + 1; worldY < p_placement.worldMin.y; ++worldY)
						{
							const spk::Vector3Int local{worldX - origin.x, worldY - origin.y, worldZ - origin.z};
							if (p_editor.isWithinBounds(local))
							{
								spk::VoxelCell cell;
								cell.id = column.deepId;
								(void)p_editor.setCell(local, cell);
							}
						}
					}
				}
			}

			// Stamp the rotated box; the prefab lists its empty cells too, so they overwrite
			// (carve) and interiors and the air above ramps stay clear even against a cliff.
			p_editor.applyPrefab(p_placement.definition->prefab, p_placement.destination - origin, p_placement.orientation);

			// A ramp's bounding box contains authored air below its higher treads. The
			// rectangular foundation pass above deliberately stops at the box, and stamping
			// then carves that authored air. Support every column that actually contains a
			// solid prefab voxel down to terrain so the stair cannot leave a diagonal gap.
			if (p_placement.foundation)
			{
				const int width = p_placement.rotatedSize.x;
				const int depth = p_placement.rotatedSize.z;
				std::vector<int> supportTops(
					static_cast<std::size_t>(width * depth),
					std::numeric_limits<int>::max());
				p_placement.definition->prefab.forEachAppliedVoxel(
					p_placement.destination,
					p_placement.orientation,
					[&](const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell) {
						if (p_cell.isEmpty())
						{
							return;
						}
						const int dx = p_position.x - p_placement.worldMin.x;
						const int dz = p_position.z - p_placement.worldMin.z;
						if (dx >= 0 && dx < width && dz >= 0 && dz < depth)
						{
							int &supportTop = supportTops[static_cast<std::size_t>(dz * width + dx)];
							supportTop = std::min(supportTop, p_position.y);
						}
					});
				for (int dz = 0; dz < depth; ++dz)
				{
					for (int dx = 0; dx < width; ++dx)
					{
						const int supportTop = supportTops[static_cast<std::size_t>(dz * width + dx)];
						if (supportTop == std::numeric_limits<int>::max())
						{
							continue;
						}
						const int worldX = p_placement.worldMin.x + dx;
						const int worldZ = p_placement.worldMin.z + dz;
						const Column column = _column(worldX, worldZ);
						for (int worldY = column.groundTop + 1; worldY < supportTop; ++worldY)
						{
							const spk::Vector3Int local{worldX - origin.x, worldY - origin.y, worldZ - origin.z};
							if (p_editor.isWithinBounds(local))
							{
								spk::VoxelCell cell;
								cell.id = column.deepId;
								(void)p_editor.setCell(local, cell);
							}
						}
					}
				}
			}
		});
	}

	int PlanChunkProvider::surfaceHeight(int p_worldX, int p_worldZ) const
	{
		return _column(p_worldX, p_worldZ).groundTop;
	}

	int PlanChunkProvider::maxHeight() const noexcept
	{
		return _plan.surfaceY(_plan.config.maxHeightLevel) + 10;
	}

	spk::Vector3Int PlanChunkProvider::spawnCell() const
	{
		const WorldGenConfig &cfg = _plan.config;
		const int blocks = cfg.blocksPerCell;
		const int offset = _plan.worldOffset();

		// Home = the gym of the first-progression zone (fallback: any gym, world center).
		int homeRow = cfg.size / 2;
		int homeCol = cfg.size / 2;
		int bestProgression = std::numeric_limits<int>::max();
		for (const PlanEntity &entity : _plan.entities)
		{
			if (entity.kind != PlanEntityKind::Gym || entity.zone < 0)
			{
				continue;
			}
			const int progression = _plan.zones[entity.zone].progression;
			if (progression < bestProgression)
			{
				bestProgression = progression;
				homeRow = entity.row;
				homeCol = entity.col;
			}
		}

		// Spawn on open road near home: at least 2 cells away so the player is not
		// inside the gym shell, and never on a cell that carries a building.
		const auto entityAt = [&](int p_row, int p_col) {
			for (const PlanEntity &entity : _plan.entities)
			{
				if (entity.row == p_row && entity.col == p_col)
				{
					return true;
				}
			}
			return false;
		};
		int spawnRow = homeRow;
		int spawnCol = homeCol;
		bool found = false;
		for (int radius = 2; radius < cfg.size && !found; ++radius)
		{
			for (int row = homeRow - radius; row <= homeRow + radius && !found; ++row)
			{
				for (int col = homeCol - radius; col <= homeCol + radius && !found; ++col)
				{
					if (std::max(std::abs(row - homeRow), std::abs(col - homeCol)) != radius)
					{
						continue;
					}
					if (_plan.road.contains(row, col) && _plan.road.at(row, col) != 0 &&
						_plan.bridge.at(row, col) == 0 && !entityAt(row, col))
					{
						spawnRow = row;
						spawnCol = col;
						found = true;
					}
				}
			}
		}

		const int worldX = offset + spawnCol * blocks + blocks / 2;
		const int worldZ = offset + spawnRow * blocks + blocks / 2;
		return {worldX, surfaceHeight(worldX, worldZ), worldZ};
	}
}
