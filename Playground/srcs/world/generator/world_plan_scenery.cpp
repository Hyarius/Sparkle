#include "world/generator/world_plan_generator.hpp"
#include "world/generator/path_surface.hpp"

#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace pg::worldgen
{
	void Generator::placeTownScenery()
	{
		struct PlacedDecor
		{
			int worldX = 0;
			int worldZ = 0;
			int spacing = 1;
		};

		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		for (const PlanTownRecord &town : plan.towns)
		{
			// The resolved origin is the blueprint bounding-box corner and may be on
			// water for a rotated port. Town ownership stays with its macro entity.
			const int zoneId = town.macroEntityIndex < plan.entities.size()
				? plan.entities[town.macroEntityIndex].zone
				: -1;
			if (zoneId < 0)
			{
				continue;
			}
			const std::vector<PlanScenery> &scenery = plan.biomes[plan.zones[zoneId].biomeIndex].townScenery;
			if (scenery.empty())
			{
				continue;
			}

			std::set<std::pair<int, int>> decorationCells;
			const std::set<std::pair<int, int>> pathCells(town.pathCells.begin(), town.pathCells.end());
			const std::set<std::pair<int, int>> boundaryCells(town.boundaryCells.begin(), town.boundaryCells.end());
			std::vector<Cell> candidates;
			for (const auto &[row, col] : town.decorationCells)
			{
				decorationCells.emplace(row, col);
				if (plan.land.contains(row, col) && isLand(row, col) && plan.water.at(row, col) == 0 &&
					plan.townPath.at(row, col) == 0)
				{
					candidates.push_back({row, col});
				}
			}
			std::vector<Cell> roadsideCandidates;
			for (const auto &[row, col] : town.pathCells)
			{
				if (plan.land.contains(row, col) && isLand(row, col) && plan.water.at(row, col) == 0)
				{
					roadsideCandidates.push_back({row, col});
				}
			}

			Rng rng = rngFor("town:" + std::to_string(town.macroEntityIndex) + "/scenery");
			std::vector<PlacedDecor> placed;
			const auto hasEnoughSpacing = [&](int p_worldX, int p_worldZ, int p_spacing) {
				return std::ranges::all_of(placed, [&](const PlacedDecor &other) {
					const int dx = p_worldX - other.worldX;
					const int dz = p_worldZ - other.worldZ;
					const int required = std::max(p_spacing, other.spacing);
					return dx * dx + dz * dz >= required * required;
				});
			};
			const auto footprintIsInCells = [&](const ResolvedPlacementBox &p_box, const std::set<std::pair<int, int>> &p_cells) {
				const int minCol = plan.cellIndexFromWorld(p_box.worldMin.x);
				const int maxCol = plan.cellIndexFromWorld(p_box.worldMin.x + p_box.extents.x - 1);
				const int minRow = plan.cellIndexFromWorld(p_box.worldMin.z);
				const int maxRow = plan.cellIndexFromWorld(p_box.worldMin.z + p_box.extents.z - 1);
				for (int row = minRow; row <= maxRow; ++row)
				{
					for (int col = minCol; col <= maxCol; ++col)
					{
						if (!p_cells.contains({row, col}) ||
							plan.height.at(row, col) != plan.height.at(minRow, minCol))
						{
							return false;
						}
					}
				}
				return true;
			};
			const auto isTownPathSurface = [&](int p_worldX, int p_worldZ) {
				const int row = plan.cellIndexFromWorld(p_worldZ);
				const int col = plan.cellIndexFromWorld(p_worldX);
				if (!pathCells.contains({row, col}))
				{
					return false;
				}
				const int localX = p_worldX - (offset + col * blocks);
				const int localZ = p_worldZ - (offset + row * blocks);
				return isCenteredPathSurface(
					blocks,
					localX,
					localZ,
					pathCells.contains({row - 1, col}),
					pathCells.contains({row + 1, col}),
					pathCells.contains({row, col - 1}),
					pathCells.contains({row, col + 1}));
			};
			const auto footprintIsRoadside = [&](const ResolvedPlacementBox &p_box) {
				bool touchesPath = false;
				for (int z = p_box.worldMin.z; z < p_box.worldMin.z + p_box.extents.z; ++z)
				{
					for (int x = p_box.worldMin.x; x < p_box.worldMin.x + p_box.extents.x; ++x)
					{
						if (isTownPathSurface(x, z))
						{
							return false;
						}
						touchesPath = touchesPath || isTownPathSurface(x - 1, z) || isTownPathSurface(x + 1, z) ||
							isTownPathSurface(x, z - 1) || isTownPathSurface(x, z + 1);
					}
				}
				return touchesPath;
			};

			// A town has only a handful of authored decoration cells, so wilderness-style
			// Bernoulli sampling made empty settlements the usual result. Every positive
			// decor entry gets one deterministic request; density controls only additional
			// instances. A density of zero still disables the entry completely.
			for (const PlanScenery &entry : scenery)
			{
				if (entry.density <= 0.0)
				{
					continue;
				}
				const std::vector<Cell> &eligibleCells = entry.roadside ? roadsideCandidates : candidates;
				if (eligibleCells.empty())
				{
					continue;
				}
				const int requestCount = 1 + rng.poisson(entry.density * static_cast<double>(eligibleCells.size()));
				for (int request = 0; request < requestCount; ++request)
				{
					std::vector<Cell> orderedCandidates = eligibleCells;
					rng.shuffle(orderedCandidates);
					bool committed = false;
					for (const Cell &cell : orderedCandidates)
					{
						for (int attempt = 0; attempt < 64; ++attempt)
						{
							const int turns = rng.below(4);
							const int worldX = offset + cell.col * blocks + rng.below(blocks);
							const int worldZ = offset + cell.row * blocks + rng.below(blocks);
							if (!hasEnoughSpacing(worldX, worldZ, entry.spacing))
							{
								continue;
							}
							PrefabPlacement placement{
								.prefabId = entry.prefabId,
								.anchor = {worldX, plan.surfaceY(plan.height.at(cell.row, cell.col)) + 1, worldZ},
								.orientation = spk::orientationFromQuarterTurns(turns),
								.foundation = false};
							const std::optional<ResolvedPlacementBox> box = resolveBox(placement);
							const std::optional<Claim> claim = claimBoxFor(placement);
							if (!box.has_value() || !claim.has_value() ||
								!footprintIsInCells(*box, entry.roadside ? boundaryCells : decorationCells) ||
								(entry.roadside && !footprintIsRoadside(*box)) ||
								!zoneIsFree(*claim))
							{
								continue;
							}
							plan.placements.push_back(std::move(placement));
							hardClaims.push_back(*claim);
							placed.push_back({worldX, worldZ, entry.spacing});
							++plan.stats.sceneryPlacements;
							committed = true;
							break;
						}
						if (committed)
						{
							break;
						}
					}
				}
			}
		}
	}

	// Scenery is biome dressing rather than a world entity. Density is the expected
	// number of instances requested per suitable plan cell; Poisson sampling keeps
	// fractional densities meaningful while allowing values above one. Placements
	// receive random voxel-level offsets and honor each prefab's center spacing.
	void Generator::placeScenery()
	{
		std::set<std::pair<int, int>> blockedCells;
		for (const PlanEntity &entity : plan.entities)
		{
			blockedCells.insert({entity.row, entity.col});
		}
		for (const Cell &cell : stairCells)
		{
			blockedCells.insert({cell.row, cell.col});
		}
		for(const PlanTownRecord &town:plan.towns) for(const auto &[row,col]:town.boundaryCells) blockedCells.insert({row,col});

		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		std::map<std::pair<int, int>, int> placedScenery;
		int maximumSpacing = 1;
		const auto hasEnoughSpacing = [&](int p_worldX, int p_worldZ, int p_spacing) {
			const int searchRadius = std::max(maximumSpacing, p_spacing);
			for (int dz = -searchRadius; dz <= searchRadius; ++dz)
			{
				for (int dx = -searchRadius; dx <= searchRadius; ++dx)
				{
					const auto found = placedScenery.find({p_worldX + dx, p_worldZ + dz});
					if (found == placedScenery.end())
					{
						continue;
					}
					const int required = std::max(p_spacing, found->second);
					if (dx * dx + dz * dz < required * required)
					{
						return false;
					}
				}
			}
			return true;
		};

		constexpr int AttemptsPerInstance = 8;
		for (const PlanZone &zone : plan.zones)
		{
			const std::vector<PlanScenery> &scenery = plan.biomes[zone.biomeIndex].scenery;
			if (scenery.empty())
			{
				continue;
			}
			std::vector<Cell> candidates;
			for (int row = 0; row < size; ++row)
			{
				for (int col = 0; col < size; ++col)
				{
					if (plan.zone.at(row, col) == zone.id && isLand(row, col) &&
						plan.road.at(row, col) == 0 && plan.water.at(row, col) == 0 &&
						!blockedCells.contains({row, col}))
					{
						candidates.push_back({row, col});
					}
				}
			}

			Rng rng = rngFor("zone:" + std::to_string(zone.id) + "/scenery");
			rng.shuffle(candidates);
			for (const Cell &candidate : candidates)
			{
				std::vector<const PlanScenery *> requests;
				for (const PlanScenery &entry : scenery)
				{
					const int count = rng.poisson(entry.density);
					requests.insert(requests.end(), static_cast<std::size_t>(count), &entry);
				}
				rng.shuffle(requests);
				for (const PlanScenery *entry : requests)
				{
					for (int attempt = 0; attempt < AttemptsPerInstance; ++attempt)
					{
						const int turns = rng.below(4);
						const int width = turns % 2 == 0 ? entry->prefabSize.x : entry->prefabSize.z;
						const int depth = turns % 2 == 0 ? entry->prefabSize.z : entry->prefabSize.x;
						const int worldX = offset + candidate.col * blocks + rng.below(blocks);
						const int worldZ = offset + candidate.row * blocks + rng.below(blocks);
						const int minCol = plan.cellIndexFromWorld(worldX - width / 2);
						const int maxCol = plan.cellIndexFromWorld(worldX - width / 2 + width - 1);
						const int minRow = plan.cellIndexFromWorld(worldZ - depth / 2);
						const int maxRow = plan.cellIndexFromWorld(worldZ - depth / 2 + depth - 1);
						bool footprintIsClear = true;
						for (int row = minRow; row <= maxRow && footprintIsClear; ++row)
						{
							for (int col = minCol; col <= maxCol; ++col)
							{
								if (!plan.zone.contains(row, col) || plan.zone.at(row, col) != zone.id ||
									!isLand(row, col) || plan.road.at(row, col) != 0 || plan.water.at(row, col) != 0 ||
									plan.height.at(row, col) != plan.height.at(candidate.row, candidate.col) ||
									blockedCells.contains({row, col}))
								{
									footprintIsClear = false;
									break;
								}
							}
						}
						if (!footprintIsClear || !hasEnoughSpacing(worldX, worldZ, entry->spacing))
						{
							continue;
						}

						PrefabPlacement placement{.prefabId = entry->prefabId, .anchor = {worldX, plan.surfaceY(plan.height.at(candidate.row, candidate.col)) + 1, worldZ}, .orientation = spk::orientationFromQuarterTurns(turns), .foundation = false};
						// Scenery yields to every structural claim (stairways,
						// buildings) but keeps its own lighter spacing rules.
						const std::optional<Claim> claim = claimBoxFor(placement);
						if (claim.has_value() && !zoneIsFree(*claim))
						{
							continue;
						}
						plan.placements.push_back(std::move(placement));
						placedScenery.emplace(std::pair{worldX, worldZ}, entry->spacing);
						maximumSpacing = std::max(maximumSpacing, entry->spacing);
						++plan.stats.sceneryPlacements;
						break;
					}
				}
			}
		}
	}
}
