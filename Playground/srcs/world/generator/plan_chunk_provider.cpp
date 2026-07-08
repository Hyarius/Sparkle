#include "world/generator/plan_chunk_provider.hpp"

#include "core/registries.hpp"
#include "voxel/voxel_cell.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/prefab_definition.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace pg
{
	PlanChunkProvider::PlanChunkProvider(const Registries &p_registries, const WorldPlan &p_plan) :
		_plan(p_plan)
	{
		const VoxelRegistry &voxels = p_registries.voxels();
		_road = voxels.numericId("road-block");
		_water = voxels.numericId("water");
		_sand = voxels.numericId("sand-block");
		_stone = voxels.numericId("stone-block");

		_biomeBlocks.reserve(_plan.biomes.size());
		for (const PlanBiome &biome : _plan.biomes)
		{
			const BiomeDefinition &definition = p_registries.biomes().get(biome.id);
			_biomeBlocks.push_back({.surface = voxels.numericId(definition.palette.surface), .subsurface = voxels.numericId(definition.palette.subsurface), .deep = voxels.numericId(definition.palette.deep), .road = voxels.numericId(definition.palette.road)});
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
			// anchor.x/z is the footprint center and anchor.y the ground level. The
			// rotated bounding box is re-anchored on those regardless of where the
			// prefab's pivot sits, and content below the ground level (floor slabs,
			// POI pedestals) sinks into the terrain.
			const auto [rotatedMin, rotatedMax] = prefab->prefab.rotatedBounds(placement.orientation);
			const spk::Vector3Int extents = rotatedMax - rotatedMin + spk::Vector3Int{1, 1, 1};
			const spk::Vector3Int worldMin = placement.anchorToPivot
												 ? placement.anchor + rotatedMin
												 : spk::Vector3Int{
													   placement.anchor.x - extents.x / 2,
													   placement.anchor.y + rotatedMin.y,
													   placement.anchor.z - extents.z / 2};
			_placements.push_back(
				{.definition = prefab,
				 .worldMin = worldMin,
				 .rotatedSize = extents,
				 .destination = placement.anchorToPivot ? placement.anchor : worldMin - rotatedMin,
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
			zone >= 0 ? _biomeBlocks[_plan.zones[zone].biomeIndex] : BiomeBlocks{_stone, _stone, _stone, _road};
		column.groundTop = surface;
		column.surfaceId = blocksOfBiome.surface;
		column.subsurfaceId = blocksOfBiome.subsurface;
		column.deepId = blocksOfBiome.deep;

		// Local position inside the plan cell + membership in the 3-wide central strip.
		const int localX = p_worldX - (offset + col * blocks);
		const int localZ = p_worldZ - (offset + row * blocks);
		const int strip = blocks / 2 - 1;
		const bool inStripX = localX >= strip && localX <= strip + 2;
		const bool inStripZ = localZ >= strip && localZ <= strip + 2;

		// A strip cross: the center pad plus one arm toward each connected neighbor.
		const auto inCross = [&](bool p_north, bool p_south, bool p_west, bool p_east) {
			if (inStripX && inStripZ)
			{
				return true;
			}
			if (inStripX && ((p_north && localZ <= strip + 2) || (p_south && localZ >= strip)))
			{
				return true;
			}
			if (inStripZ && ((p_west && localX <= strip + 2) || (p_east && localX >= strip)))
			{
				return true;
			}
			return false;
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

		if (_plan.road.at(row, col) != 0)
		{
			const auto paved = [&](int p_row, int p_col) {
				return _plan.road.contains(p_row, p_col) && _plan.road.at(p_row, p_col) != 0;
			};
			if (inCross(paved(row - 1, col), paved(row + 1, col), paved(row, col - 1), paved(row, col + 1)))
			{
				// On bridges the deck sits at ground level on a solid pier; the water
				// column underneath is filled, its neighbors keep flowing around it.
				column.groundTop = surface;
				column.surfaceId = blocksOfBiome.road;
				column.subsurfaceId = column.deepId;
				column.waterY = -1;
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

		for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
		{
			for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
			{
				const Column column = _column(origin.x + x, origin.z + z);
				for (int y = 0; y < spk::VoxelChunk::Size.y; ++y)
				{
					const int worldY = origin.y + y;
					VoxelCell value;
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
					p_chunk.grid().cell(x, y, z) = value;
				}
			}
		}

		const spk::Vector3Int chunkMin = origin;
		const spk::Vector3Int chunkMax = origin + spk::VoxelChunk::Size;
		for (const ResolvedPlacement &placement : _placements)
		{
			const bool intersects = placement.worldMin.x < chunkMax.x &&
									placement.worldMin.x + placement.rotatedSize.x > chunkMin.x &&
									placement.worldMin.z < chunkMax.z &&
									placement.worldMin.z + placement.rotatedSize.z > chunkMin.z &&
									placement.worldMin.y < chunkMax.y &&
									placement.worldMin.y + placement.rotatedSize.y > chunkMin.y - 32;
			if (intersects)
			{
				_stamp(p_chunk, placement);
			}
		}
	}

	void PlanChunkProvider::_stamp(spk::VoxelChunk &p_chunk, const ResolvedPlacement &p_placement) const
	{
		const spk::Vector3Int origin = p_chunk.worldOrigin();

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
						if (p_chunk.grid().isWithinBounds(local))
						{
							VoxelCell cell;
							cell.id = column.deepId;
							p_chunk.grid().cell(local) = cell;
						}
					}
				}
			}
		}

		// Stamp the rotated box; the prefab lists its empty cells too, so they overwrite
		// (carve) and interiors and the air above ramps stay clear even against a cliff.
		p_placement.definition->prefab.applyTo(p_chunk.grid(), p_placement.destination - origin, p_placement.orientation);
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
