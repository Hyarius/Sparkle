#include "world/generator/world_plan_generator.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <queue>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

// Stages A-D of the world plan: the zone graph, the landmass and its zones, the
// strata heights, and hydrology.
namespace pg::worldgen
{
	// ---------------- Stage A: world graph ----------------
	void Generator::buildWorldGraph()
	{
		Rng rng = rngFor("world/graph");
		std::vector<int> biomeIndices(plan.biomes.size());
		for (std::size_t index = 0; index < biomeIndices.size(); ++index)
		{
			biomeIndices[index] = static_cast<int>(index);
		}
		rng.shuffle(biomeIndices);
		std::vector<int> order(cfg.zoneCount);
		for (int index = 0; index < cfg.zoneCount; ++index)
		{
			order[index] = index;
		}
		rng.shuffle(order);
		for (int index = 0; index < cfg.zoneCount; ++index)
		{
			plan.zones.push_back({.id = index, .biomeIndex = biomeIndices[index % biomeIndices.size()], .progression = order[index]});
		}
	}

	// ---------------- Stage B: landmass + zones ----------------
	void Generator::buildLandmass()
	{
		Rng rng = rngFor("world/landmass");
		const double center = (size - 1) / 2.0;
		Field field(size);
		const Field noise = valueNoise(rng, size, std::max(4.0, size / 12.0));
		const Field fine = valueNoise(rng, size, std::max(2.0, size / 26.0));
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				const double radius = std::hypot(j - center, i - center) / (size / 2.0);
				field.at(i, j) = (1.0 - radius) + cfg.coastNoise * (noise.at(i, j) - 0.5) + 0.25 * (fine.at(i, j) - 0.5);
			}
		}
		if (cfg.fragmentation > 0.0)
		{
			const int channels = 1 + static_cast<int>(cfg.fragmentation * 2.0);
			for (int channel = 0; channel < channels; ++channel)
			{
				Rng channelRng = rngFor("world/landmass/channel:" + std::to_string(channel));
				const double angle = channelRng.uniform() * 3.14159265358979323846;
				const double nx = std::cos(angle);
				const double ny = std::sin(angle);
				const Field wobbleNoise = valueNoise(channelRng, size, std::max(3.0, size / 10.0));
				const double sigma = size * 0.035;
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						const double signedDistance = (j - center) * nx + (i - center) * ny;
						const double wobble = (wobbleNoise.at(i, j) - 0.5) * size * 0.18;
						const double trough = std::exp(-((signedDistance + wobble) * (signedDistance + wobble)) / (2.0 * sigma * sigma));
						field.at(i, j) -= cfg.fragmentation * 0.9 * trough;
					}
				}
			}
		}
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				plan.land.at(i, j) = field.at(i, j) > cfg.landThreshold ? 1 : 0;
			}
		}
		dropTinyIslands();
		continents = labelComponents(plan.land, continentCount);

		Mask ocean(size, 0);
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				ocean.at(i, j) = isLand(i, j) ? 0 : 1;
			}
		}
		distOcean = distanceTo(ocean);
	}

	void Generator::dropTinyIslands()
	{
		int count = 0;
		const PlanGrid<int> component = labelComponents(plan.land, count);
		if (count == 0)
		{
			return;
		}
		std::vector<int> sizes(count + 1, 0);
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				++sizes[component.at(i, j)];
			}
		}
		sizes[0] = 0;
		const int minSize = std::max(20, static_cast<int>(0.01 * size * size));
		std::set<int> keep;
		for (int id = 1; id <= count; ++id)
		{
			if (sizes[id] >= minSize)
			{
				keep.insert(id);
			}
		}
		if (keep.empty())
		{
			keep.insert(static_cast<int>(std::max_element(sizes.begin(), sizes.end()) - sizes.begin()));
		}
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				plan.land.at(i, j) = keep.contains(component.at(i, j)) ? 1 : 0;
			}
		}
	}

	std::vector<Cell> Generator::landCells() const
	{
		std::vector<Cell> cells;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (isLand(i, j))
				{
					cells.push_back({i, j});
				}
			}
		}
		return cells;
	}

	void Generator::assignZones()
	{
		Rng rng = rngFor("world/territory");
		const std::vector<Cell> land = landCells();
		if (land.empty())
		{
			return;
		}
		const std::vector<Cell> seeds = farthestPointSeeds(land, cfg.zoneCount, rng);
		const Field warpY = valueNoise(rng, size, std::max(3.0, size / 14.0));
		const Field warpX = valueNoise(rng, size, std::max(3.0, size / 14.0));
		for (const Cell &cell : land)
		{
			const double wy = cell.row + (warpY.at(cell.row, cell.col) - 0.5) * size * 0.12;
			const double wx = cell.col + (warpX.at(cell.row, cell.col) - 0.5) * size * 0.12;
			int bestZone = 0;
			double bestDistance = INF;
			for (int zone = 0; zone < static_cast<int>(seeds.size()); ++zone)
			{
				const double dy = wy - seeds[zone].row;
				const double dx = wx - seeds[zone].col;
				const double distance = dy * dy + dx * dx;
				if (distance < bestDistance)
				{
					bestDistance = distance;
					bestZone = zone;
				}
			}
			plan.zone.at(cell.row, cell.col) = static_cast<std::int16_t>(bestZone);
		}
		enforceContiguousZones();
		mapZoneContinents();
		findBorders();
	}

	// Reassign small disconnected fragments of a zone to a bordering zone so each
	// zone stays (mostly) one blob.
	void Generator::enforceContiguousZones()
	{
		for (int pass = 0; pass < 2; ++pass)
		{
			PlanGrid<int> component(size, -1);
			int componentCount2 = 0;
			std::map<int, int> componentZone;
			std::map<int, int> componentSize;
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (plan.zone.at(i, j) < 0 || component.at(i, j) != -1)
					{
						continue;
					}
					const int zone = plan.zone.at(i, j);
					const int id = ++componentCount2;
					std::deque<Cell> queue{{i, j}};
					component.at(i, j) = id;
					int cells = 0;
					while (!queue.empty())
					{
						const Cell cell = queue.front();
						queue.pop_front();
						++cells;
						forNeighbors4(cell.row, cell.col, size, [&](int p_row, int p_col) {
							if (plan.zone.at(p_row, p_col) == zone && component.at(p_row, p_col) == -1)
							{
								component.at(p_row, p_col) = id;
								queue.push_back({p_row, p_col});
							}
						});
					}
					componentZone[id] = zone;
					componentSize[id] = cells;
				}
			}
			std::map<int, int> mainComponent;
			for (const auto &[id, zone] : componentZone)
			{
				if (!mainComponent.contains(zone) || componentSize[id] > componentSize[mainComponent[zone]])
				{
					mainComponent[zone] = id;
				}
			}
			bool changed = false;
			for (const auto &[id, zone] : componentZone)
			{
				if (mainComponent[zone] == id)
				{
					continue;
				}
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						if (component.at(i, j) != id)
						{
							continue;
						}
						int replacement = -1;
						forNeighbors4(i, j, size, [&](int p_row, int p_col) {
							const int other = plan.zone.at(p_row, p_col);
							if (replacement == -1 && other >= 0 && other != zone)
							{
								replacement = other;
							}
						});
						if (replacement != -1)
						{
							plan.zone.at(i, j) = static_cast<std::int16_t>(replacement);
							changed = true;
						}
					}
				}
			}
			if (!changed)
			{
				break;
			}
		}
	}

	void Generator::mapZoneContinents()
	{
		std::map<int, std::map<int, int>> counts;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				const int zone = plan.zone.at(i, j);
				if (zone >= 0 && continents.at(i, j) > 0)
				{
					++counts[zone][continents.at(i, j)];
				}
			}
		}
		for (const auto &[zone, byContinent] : counts)
		{
			int best = 1;
			int bestCount = -1;
			for (const auto &[continent, count] : byContinent)
			{
				if (count > bestCount)
				{
					bestCount = count;
					best = continent;
				}
			}
			zoneContinent[zone] = best;
		}
	}

	void Generator::findBorders()
	{
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				const int zone = plan.zone.at(i, j);
				if (zone < 0)
				{
					continue;
				}
				for (const auto &[nr, nc] : {std::pair{i + 1, j}, {i, j + 1}})
				{
					if (nr >= size || nc >= size)
					{
						continue;
					}
					const int other = plan.zone.at(nr, nc);
					if (other >= 0 && other != zone)
					{
						borders[{std::min(zone, other), std::max(zone, other)}].push_back({i, j});
					}
				}
			}
		}
	}

	// ---------------- Stage C: heights ----------------
	void Generator::assignHeights()
	{
		Rng rng = rngFor("world/heights");
		const double cellsPerLevel = cfg.cellsPerLevel;
		const double coastLevels = cfg.maxHeightLevel * cfg.coastTrendWeight;
		const double coastReach = std::max(1.0, cellsPerLevel * coastLevels);

		const double feature = std::max(6.0, cfg.heightFeatureCells);
		const Field n1 = valueNoise(rng, size, feature);
		const Field n2 = valueNoise(rng, size, feature * 0.5);
		const Field n3 = valueNoise(rng, size, feature * 0.25);

		// Peaks: extra lift near a few summits so peak-flagged biomes tower.
		std::vector<int> peakZones;
		for (const PlanZone &zone : plan.zones)
		{
			if (plan.biomes[zone.biomeIndex].peak)
			{
				peakZones.push_back(zone.id);
			}
		}
		if (peakZones.empty())
		{
			for (const PlanZone &zone : plan.zones)
			{
				peakZones.push_back(zone.id);
			}
		}
		const int peakCount = std::max(2, static_cast<int>(std::lround((size / 124.0) * (size / 124.0) * 2.0)));
		Mask peakMask(size, 0);
		bool anyPeak = false;
		for (int peak = 0; peak < peakCount; ++peak)
		{
			const int zone = peakZones[rng.below(static_cast<int>(peakZones.size()))];
			std::vector<Cell> zoneCells;
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (plan.zone.at(i, j) == zone && isLand(i, j))
					{
						zoneCells.push_back({i, j});
					}
				}
			}
			if (zoneCells.empty())
			{
				continue;
			}
			std::vector<int> depths;
			depths.reserve(zoneCells.size());
			for (const Cell &cell : zoneCells)
			{
				depths.push_back(distOcean.at(cell.row, cell.col));
			}
			std::vector<int> sorted = depths;
			std::sort(sorted.begin(), sorted.end());
			const int threshold = sorted[static_cast<std::size_t>(0.6 * (sorted.size() - 1))];
			std::vector<Cell> inland;
			for (std::size_t index = 0; index < zoneCells.size(); ++index)
			{
				if (depths[index] >= threshold)
				{
					inland.push_back(zoneCells[index]);
				}
			}
			if (!inland.empty())
			{
				const Cell chosen = inland[rng.below(static_cast<int>(inland.size()))];
				peakMask.at(chosen.row, chosen.col) = 1;
				anyPeak = true;
			}
		}
		if (!anyPeak)
		{
			const std::vector<Cell> land = landCells();
			if (!land.empty())
			{
				const Cell chosen = land[rng.below(static_cast<int>(land.size()))];
				peakMask.at(chosen.row, chosen.col) = 1;
			}
		}
		const PlanGrid<int> distPeak = distanceTo(peakMask);
		const double peakRadius = std::max(1.0, cellsPerLevel * cfg.maxHeightLevel * 0.7);

		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (!isLand(i, j))
				{
					continue;
				}
				const double dCoast = distOcean.at(i, j);
				const double coastTrend = coastLevels * (1.0 - std::exp(-dCoast / coastReach));
				const double undulation =
					((n1.at(i, j) - 0.5) + 0.5 * (n2.at(i, j) - 0.5) + 0.25 * (n3.at(i, j) - 0.5)) / 1.75 * 2.0 *
					cfg.undulationLevels;
				const double lift = std::pow(std::clamp(1.0 - distPeak.at(i, j) / peakRadius, 0.0, 1.0), 1.5) *
									(cfg.maxHeightLevel * 0.65);
				const int zone = plan.zone.at(i, j);
				const PlanBiome *biome = zone >= 0 ? &plan.biomes[plan.zones[zone].biomeIndex] : nullptr;
				const double weight = (biome != nullptr && biome->peak) ? 1.0 : 0.25;
				const double shift = biome != nullptr ? biome->heightShift : 0.0;
				const double field = coastTrend + undulation + lift * weight + shift;
				int level = static_cast<int>(std::floor(field));
				level = std::clamp(level, 0, cfg.maxHeightLevel);
				plan.height.at(i, j) = static_cast<std::int8_t>(level);
			}
		}
	}

	// ---------------- Stage D: hydrology ----------------
	void Generator::generateWater()
	{
		if (!cfg.enableRivers)
		{
			return;
		}
		Rng rng = rngFor("world/water");
		const Field jitter = valueNoise(rng, size, 3.0);
		Field effective(size);
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				effective.at(i, j) = plan.height.at(i, j) + jitter.at(i, j) * 0.30 + rng.uniform() * 0.12;
				if (!isLand(i, j))
				{
					effective.at(i, j) = -1e6; // ocean is lowest: rivers always want the sea
				}
			}
		}

		// Depression filling (priority flood). On the filled surface every land cell
		// drains to the ocean; `receiver` records the downstream neighbor.
		Field filled(size, INF);
		PlanGrid<int> receiver(size, -1);
		Mask visited(size, 0);
		using Node = std::tuple<double, int>; // (level, row*size+col); receiver set at pop
		std::priority_queue<std::pair<Node, int>, std::vector<std::pair<Node, int>>, std::greater<>> queue;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (!isLand(i, j))
				{
					filled.at(i, j) = effective.at(i, j);
					visited.at(i, j) = 1;
				}
			}
		}
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (isLand(i, j))
				{
					continue;
				}
				forNeighbors4(i, j, size, [&](int p_row, int p_col) {
					if (visited.at(p_row, p_col) == 0)
					{
						visited.at(p_row, p_col) = 1;
						queue.push({{effective.at(p_row, p_col), p_row * size + p_col}, i * size + j});
					}
				});
			}
		}
		while (!queue.empty())
		{
			const auto [node, from] = queue.top();
			queue.pop();
			const auto [level, index] = node;
			const int row = index / size;
			const int col = index % size;
			filled.at(row, col) = std::max(effective.at(row, col), level);
			receiver.at(row, col) = from;
			forNeighbors4(row, col, size, [&](int p_row, int p_col) {
				if (visited.at(p_row, p_col) == 0)
				{
					visited.at(p_row, p_col) = 1;
					queue.push({{std::max(effective.at(p_row, p_col), filled.at(row, col)), p_row * size + p_col}, index});
				}
			});
		}

		// Keep only deep-and-large filled basins as lakes; drain the rest. Oversized
		// basins are trimmed to a connected core grown from their deepest cell.
		Mask deep(size, 0);
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (isLand(i, j) && (filled.at(i, j) - effective.at(i, j)) >= cfg.lakeMinDepth)
				{
					deep.at(i, j) = 1;
				}
			}
		}
		int deepCount = 0;
		const PlanGrid<int> deepComponent = labelComponents(deep, deepCount);
		for (int id = 1; id <= deepCount; ++id)
		{
			std::vector<Cell> cells;
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (deepComponent.at(i, j) == id)
					{
						cells.push_back({i, j});
					}
				}
			}
			if (static_cast<int>(cells.size()) < cfg.lakeMinSize)
			{
				continue;
			}
			const auto depthOf = [&](const Cell &p_cell) {
				return filled.at(p_cell.row, p_cell.col) - effective.at(p_cell.row, p_cell.col);
			};
			if (static_cast<int>(cells.size()) > cfg.lakeMaxSize)
			{
				std::set<std::pair<int, int>> members;
				for (const Cell &cell : cells)
				{
					members.insert({cell.row, cell.col});
				}
				const Cell seed = *std::max_element(cells.begin(), cells.end(), [&](const Cell &p_a, const Cell &p_b) {
					return depthOf(p_a) < depthOf(p_b);
				});
				std::vector<Cell> core;
				std::deque<Cell> queue2{seed};
				std::set<std::pair<int, int>> seen{{seed.row, seed.col}};
				while (!queue2.empty() && static_cast<int>(core.size()) < cfg.lakeMaxSize)
				{
					const Cell cell = queue2.front();
					queue2.pop_front();
					core.push_back(cell);
					std::vector<Cell> next;
					forNeighbors4(cell.row, cell.col, size, [&](int p_row, int p_col) {
						if (members.contains({p_row, p_col}) && !seen.contains({p_row, p_col}))
						{
							next.push_back({p_row, p_col});
						}
					});
					std::sort(next.begin(), next.end(), [&](const Cell &p_a, const Cell &p_b) {
						return depthOf(p_a) > depthOf(p_b);
					});
					for (const Cell &candidate : next)
					{
						seen.insert({candidate.row, candidate.col});
						queue2.push_back(candidate);
					}
				}
				cells = core;
			}
			for (const Cell &cell : cells)
			{
				plan.water.at(cell.row, cell.col) = 1;
				plan.lake.at(cell.row, cell.col) = 1;
			}
		}

		// Sources: rivers start high and inland, spread apart, per zone.
		std::vector<Cell> sources;
		if (cfg.riversPerZone > 0.0)
		{
			for (const PlanZone &zone : plan.zones)
			{
				Rng zoneRng = rngFor("zone:" + std::to_string(zone.id) + "/river_sources");
				const int base = static_cast<int>(cfg.riversPerZone);
				const double fraction = cfg.riversPerZone - base;
				const int count = base + (zoneRng.uniform() < fraction ? 1 : 0);
				if (count <= 0)
				{
					continue;
				}
				std::vector<Cell> candidates;
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						if (plan.zone.at(i, j) == zone.id && isLand(i, j) && distOcean.at(i, j) > 3)
						{
							candidates.push_back({i, j});
						}
					}
				}
				if (candidates.empty())
				{
					for (int i = 0; i < size; ++i)
					{
						for (int j = 0; j < size; ++j)
						{
							if (plan.zone.at(i, j) == zone.id && isLand(i, j))
							{
								candidates.push_back({i, j});
							}
						}
					}
				}
				if (candidates.empty())
				{
					continue;
				}
				std::sort(candidates.begin(), candidates.end(), [&](const Cell &p_a, const Cell &p_b) {
					return filled.at(p_a.row, p_a.col) > filled.at(p_b.row, p_b.col);
				});
				const int poolSize = std::min<int>(static_cast<int>(candidates.size()), std::max(count * 15, 30));
				candidates.resize(poolSize);
				for (const Cell &picked : farthestPointSeeds(candidates, count, zoneRng))
				{
					sources.push_back(picked);
				}
			}
		}

		// Walk each source down the drainage tree until the sea or existing water.
		for (const Cell &source : sources)
		{
			Cell cursor = source;
			std::set<std::pair<int, int>> seen;
			std::vector<Cell> channel;
			for (int step = 0; step < size * size; ++step)
			{
				if (!isLand(cursor.row, cursor.col))
				{
					break;
				}
				if (plan.water.at(cursor.row, cursor.col) != 0 && !(cursor == source))
				{
					break;
				}
				if (seen.contains({cursor.row, cursor.col}))
				{
					break;
				}
				seen.insert({cursor.row, cursor.col});
				channel.push_back(cursor);
				const int next = receiver.at(cursor.row, cursor.col);
				if (next < 0)
				{
					break;
				}
				cursor = {next / size, next % size};
			}
			for (const Cell &cell : channel)
			{
				if (isLand(cell.row, cell.col))
				{
					plan.water.at(cell.row, cell.col) = 1;
				}
			}
		}
	}
}
