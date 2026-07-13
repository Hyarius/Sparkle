#include "world/generator/world_plan_generator.hpp"

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
		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		constexpr std::array<std::pair<int, int>, 4> neighbors = {{{-1, 0}, {0, -1}, {0, 1}, {1, 0}}};
		for (const PlanZone &zone : plan.zones)
		{
			const std::vector<PlanScenery> &scenery = plan.biomes[zone.biomeIndex].townScenery;
			if (scenery.empty())
			{
				continue;
			}
			std::vector<Cell> candidates;
			for (int row = 0; row < size; ++row)
			{
				for (int col = 0; col < size; ++col)
				{
					if (plan.zone.at(row, col) != zone.id || !isLand(row, col) || plan.water.at(row, col) != 0 || plan.townRoad.at(row, col) != 0)
					{
						continue;
					}
					const bool besideTownRoad = std::ranges::any_of(neighbors, [&](const auto &p_delta) {
						const int nr = row + p_delta.first;
						const int nc = col + p_delta.second;
						return plan.townRoad.contains(nr, nc) && plan.townRoad.at(nr, nc) != 0;
					});
					if (besideTownRoad)
					{
						candidates.push_back({row, col});
					}
				}
			}
			Rng rng = rngFor("zone:" + std::to_string(zone.id) + "/town_scenery");
			rng.shuffle(candidates);
			for (const Cell &cell : candidates)
			{
				for (const PlanScenery &entry : scenery)
				{
					if (rng.poisson(entry.density) == 0)
					{
						continue;
					}
					const int turns = rng.below(4);
					PrefabPlacement placement{.prefabId = entry.prefabId,
						.anchor = {offset + cell.col * blocks + blocks / 2, plan.surfaceY(plan.height.at(cell.row, cell.col)) + 1, offset + cell.row * blocks + blocks / 2},
						.orientation = spk::orientationFromQuarterTurns(turns), .foundation = false};
					const std::optional<Claim> claim = claimBoxFor(placement);
					if (!claim.has_value() || !zoneIsFree(*claim))
					{
						continue;
					}
					plan.placements.push_back(std::move(placement));
					hardClaims.push_back(*claim);
					++plan.stats.sceneryPlacements;
					break;
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
