#include "world/generator/macro_world_generator.hpp"

#include <structures/math/spk_perlin.hpp>
#include <structures/math/spk_poisson_disk.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace
{
	using pg::MacroWorldPlan;
	using pg::PlanCell;
	using pg::PlanFeatureType;
	using pg::TransportClass;
	using pg::WorldgenParams;

	constexpr std::array<PlanCell, 8> Neighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};
	constexpr std::array<PlanCell, 4> CardinalNeighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};

	std::uint64_t splitMix64(std::uint64_t p_value) noexcept
	{
		p_value += 0x9e3779b97f4a7c15ULL;
		p_value = (p_value ^ (p_value >> 30)) * 0xbf58476d1ce4e5b9ULL;
		p_value = (p_value ^ (p_value >> 27)) * 0x94d049bb133111ebULL;
		return p_value ^ (p_value >> 31);
	}

	std::mt19937_64 stageRandom(std::uint64_t p_runSeed, std::uint64_t p_stage)
	{
		return std::mt19937_64(splitMix64(p_runSeed ^ (p_stage * 0x9e3779b97f4a7c15ULL)));
	}

	float distanceSquared(PlanCell p_a, PlanCell p_b) noexcept
	{
		const float dx = static_cast<float>(p_a.x - p_b.x);
		const float dy = static_cast<float>(p_a.y - p_b.y);
		return dx * dx + dy * dy;
	}

	void retainMajorLandmassesAndBuildOcean(MacroWorldPlan &p_plan, int p_maxLandmasses)
	{
		const std::size_t cellCount = p_plan.landMask.size();
		std::vector<int> component(cellCount, -1);
		std::vector<std::size_t> componentSizes;
		std::queue<std::size_t> pending;
		int nextComponent = 0;
		for (std::size_t start = 0; start < cellCount; ++start)
		{
			if (!p_plan.landMask[start] || component[start] >= 0)
			{
				continue;
			}
			componentSizes.push_back(0);
			component[start] = nextComponent;
			pending.push(start);
			while (!pending.empty())
			{
				const std::size_t index = pending.front();
				pending.pop();
				++componentSizes.back();
				const PlanCell cell{
					static_cast<int>(index % static_cast<std::size_t>(p_plan.width)),
					static_cast<int>(index / static_cast<std::size_t>(p_plan.width))};
				for (const PlanCell offset : CardinalNeighbours)
				{
					const PlanCell neighbour{cell.x + offset.x, cell.y + offset.y};
					if (!p_plan.contains(neighbour))
					{
						continue;
					}
					const std::size_t neighbourIndex = p_plan.index(neighbour);
					if (!p_plan.landMask[neighbourIndex] || component[neighbourIndex] >= 0)
					{
						continue;
					}
					component[neighbourIndex] = nextComponent;
					pending.push(neighbourIndex);
				}
			}
			++nextComponent;
		}
		if (componentSizes.empty())
		{
			throw std::runtime_error("world generation produced no landmass");
		}
		std::vector<int> orderedComponents(componentSizes.size());
		std::iota(orderedComponents.begin(), orderedComponents.end(), 0);
		std::ranges::sort(orderedComponents, [&](int p_left, int p_right) {
			return componentSizes[static_cast<std::size_t>(p_left)] > componentSizes[static_cast<std::size_t>(p_right)];
		});
		std::vector<std::int16_t> componentToLandmass(componentSizes.size(), -1);
		const std::size_t retainedCount = std::min<std::size_t>(
			orderedComponents.size(), static_cast<std::size_t>(std::max(1, p_maxLandmasses)));
		for (std::size_t landmass = 0; landmass < retainedCount; ++landmass)
		{
			componentToLandmass[static_cast<std::size_t>(orderedComponents[landmass])] = static_cast<std::int16_t>(landmass);
		}
		for (std::size_t index = 0; index < cellCount; ++index)
		{
			const int componentId = component[index];
			const std::int16_t landmass = componentId < 0 ? -1 : componentToLandmass[static_cast<std::size_t>(componentId)];
			p_plan.landmassCells[index] = landmass;
			p_plan.landMask[index] = landmass >= 0 ? 1 : 0;
		}

		std::ranges::fill(p_plan.oceanMask, 0);
		const auto enqueueOcean = [&](PlanCell p_cell) {
			if (!p_plan.contains(p_cell))
			{
				return;
			}
			const std::size_t index = p_plan.index(p_cell);
			if (p_plan.landMask[index] || p_plan.oceanMask[index])
			{
				return;
			}
			p_plan.oceanMask[index] = 1;
			pending.push(index);
		};
		for (int x = 0; x < p_plan.width; ++x)
		{
			enqueueOcean({x, 0});
			enqueueOcean({x, p_plan.height - 1});
		}
		for (int y = 0; y < p_plan.height; ++y)
		{
			enqueueOcean({0, y});
			enqueueOcean({p_plan.width - 1, y});
		}
		while (!pending.empty())
		{
			const std::size_t index = pending.front();
			pending.pop();
			const PlanCell cell{
				static_cast<int>(index % static_cast<std::size_t>(p_plan.width)),
				static_cast<int>(index / static_cast<std::size_t>(p_plan.width))};
			for (const PlanCell offset : CardinalNeighbours)
			{
				enqueueOcean({cell.x + offset.x, cell.y + offset.y});
			}
		}

		const std::uint16_t unreachable = std::numeric_limits<std::uint16_t>::max();
		std::ranges::fill(p_plan.oceanDistances, unreachable);
		for (std::size_t index = 0; index < cellCount; ++index)
		{
			if (!p_plan.oceanMask[index])
			{
				continue;
			}
			p_plan.oceanDistances[index] = 0;
			pending.push(index);
		}
		while (!pending.empty())
		{
			const std::size_t index = pending.front();
			pending.pop();
			const PlanCell cell{
				static_cast<int>(index % static_cast<std::size_t>(p_plan.width)),
				static_cast<int>(index / static_cast<std::size_t>(p_plan.width))};
			for (const PlanCell offset : CardinalNeighbours)
			{
				const PlanCell neighbour{cell.x + offset.x, cell.y + offset.y};
				if (!p_plan.contains(neighbour))
				{
					continue;
				}
				const std::size_t neighbourIndex = p_plan.index(neighbour);
				const auto candidate = static_cast<std::uint16_t>(
					std::min<int>(unreachable - 1, static_cast<int>(p_plan.oceanDistances[index]) + 1));
				if (candidate >= p_plan.oceanDistances[neighbourIndex])
				{
					continue;
				}
				p_plan.oceanDistances[neighbourIndex] = candidate;
				pending.push(neighbourIndex);
			}
		}
	}

	bool isFlatLand(const MacroWorldPlan &p_plan, PlanCell p_cell, float p_limit)
	{
		if (!p_plan.contains(p_cell) || p_plan.landMask[p_plan.index(p_cell)] == 0)
		{
			return false;
		}
		const float center = p_plan.heights[p_plan.index(p_cell)];
		for (const PlanCell offset : Neighbours)
		{
			const PlanCell next{p_cell.x + offset.x, p_cell.y + offset.y};
			if (!p_plan.contains(next) || p_plan.landMask[p_plan.index(next)] == 0)
			{
				return false;
			}
			if (std::abs(p_plan.heights[p_plan.index(next)] - center) > p_limit)
			{
				return false;
			}
		}
		return true;
	}

	int waterDistance(const MacroWorldPlan &p_plan, PlanCell p_cell, int p_limit)
	{
		if (!p_plan.contains(p_cell))
		{
			return 0;
		}
		return std::min<int>(p_plan.oceanDistances[p_plan.index(p_cell)], p_limit + 1);
	}

	std::vector<PlanCell> rasterLine(PlanCell p_from, PlanCell p_to)
	{
		std::vector<PlanCell> result;
		int x = p_from.x;
		int y = p_from.y;
		const int dx = std::abs(p_to.x - p_from.x);
		const int sx = p_from.x < p_to.x ? 1 : -1;
		const int dy = -std::abs(p_to.y - p_from.y);
		const int sy = p_from.y < p_to.y ? 1 : -1;
		int error = dx + dy;
		while (true)
		{
			result.push_back({x, y});
			if (x == p_to.x && y == p_to.y)
			{
				break;
			}
			const int twice = error * 2;
			if (twice >= dy)
			{
				error += dy;
				x += sx;
			}
			if (twice <= dx)
			{
				error += dx;
				y += sy;
			}
		}
		return result;
	}

	TransportClass classifyPath(const MacroWorldPlan &p_plan, const WorldgenParams &p_params, const std::vector<PlanCell> &p_path)
	{
		bool water = false;
		bool river = false;
		float slopeCost = 0.0f;
		for (std::size_t i = 0; i < p_path.size(); ++i)
		{
			const std::size_t index = p_plan.index(p_path[i]);
			water = water || p_plan.landMask[index] == 0;
			river = river || p_plan.riverMask[index] != 0;
			if (i > 0)
			{
				slopeCost += std::abs(p_plan.heights[index] - p_plan.heights[p_plan.index(p_path[i - 1])]) * p_params.transport.slopeCostFactor;
			}
		}
		if (water)
		{
			return TransportClass::Sea;
		}
		if (river)
		{
			return TransportClass::Bridge;
		}
		if (slopeCost > p_params.transport.tunnelTriggerCost)
		{
			return TransportClass::Tunnel;
		}
		return TransportClass::Road;
	}

	bool edgeExists(const std::vector<pg::PlanTransportEdge> &p_edges, std::size_t p_a, std::size_t p_b)
	{
		return std::ranges::any_of(p_edges, [=](const pg::PlanTransportEdge &p_edge) {
			return (p_edge.from == p_a && p_edge.to == p_b) || (p_edge.from == p_b && p_edge.to == p_a);
		});
	}

	std::vector<PlanCell> findPath(
		const MacroWorldPlan &p_plan,
		const WorldgenParams &p_params,
		PlanCell p_start,
		PlanCell p_goal,
		bool p_allowWater)
	{
		struct Entry
		{
			float estimate;
			std::size_t index;
			bool operator>(const Entry &p_other) const noexcept
			{
				return estimate > p_other.estimate;
			}
		};

		const std::size_t count = p_plan.landMask.size();
		const float infinity = std::numeric_limits<float>::infinity();
		std::vector<float> cost(count, infinity);
		std::vector<std::int32_t> previous(count, -1);
		std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
		const std::size_t startIndex = p_plan.index(p_start);
		const std::size_t goalIndex = p_plan.index(p_goal);
		cost[startIndex] = 0.0f;
		open.push({0.0f, startIndex});

		while (!open.empty())
		{
			const Entry current = open.top();
			open.pop();
			if (current.index == goalIndex)
			{
				break;
			}
			const PlanCell cell{static_cast<int>(current.index % static_cast<std::size_t>(p_plan.width)), static_cast<int>(current.index / static_cast<std::size_t>(p_plan.width))};
			for (const PlanCell offset : CardinalNeighbours)
			{
				const PlanCell next{cell.x + offset.x, cell.y + offset.y};
				if (!p_plan.contains(next))
				{
					continue;
				}
				const std::size_t nextIndex = p_plan.index(next);
				const bool land = p_plan.landMask[nextIndex] != 0;
				if (!land && !p_allowWater)
				{
					continue;
				}
				float step = 1.0f;
				if (!land)
				{
					step += 1.5f;
				}
				else
				{
					step += std::abs(p_plan.heights[nextIndex] - p_plan.heights[current.index]) * p_params.transport.slopeCostFactor;
					if (p_plan.riverMask[nextIndex])
					{
						step += p_params.transport.riverCrossingCost;
					}
				}
				const float candidate = cost[current.index] + step;
				if (candidate >= cost[nextIndex])
				{
					continue;
				}
				cost[nextIndex] = candidate;
				previous[nextIndex] = static_cast<std::int32_t>(current.index);
				const float dx = static_cast<float>(next.x - p_goal.x);
				const float dy = static_cast<float>(next.y - p_goal.y);
				open.push({candidate + std::sqrt(dx * dx + dy * dy), nextIndex});
			}
		}

		if (!std::isfinite(cost[goalIndex]))
		{
			return {};
		}
		std::vector<PlanCell> result;
		for (std::int32_t cursor = static_cast<std::int32_t>(goalIndex); cursor >= 0; cursor = previous[static_cast<std::size_t>(cursor)])
		{
			const std::size_t index = static_cast<std::size_t>(cursor);
			result.push_back({static_cast<int>(index % static_cast<std::size_t>(p_plan.width)), static_cast<int>(index / static_cast<std::size_t>(p_plan.width))});
			if (index == startIndex)
			{
				break;
			}
		}
		std::ranges::reverse(result);
		return result;
	}

	void addFeature(MacroWorldPlan &p_plan, PlanFeatureType p_type, PlanCell p_cell, std::size_t p_edge)
	{
		const auto duplicate = std::ranges::find_if(p_plan.features, [=](const pg::PlanFeature &p_feature) {
			return p_feature.type == p_type && p_feature.cell == p_cell && p_feature.edge == p_edge;
		});
		if (duplicate != p_plan.features.end())
		{
			return;
		}
		p_plan.features.push_back({p_type, p_cell, p_edge});
		p_plan.featureMask[p_plan.index(p_cell)] = static_cast<std::uint8_t>(p_type) + 1;
	}
}

namespace pg
{
	MacroWorldPlan MacroWorldGenerator::generate(const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		p_params.validate();
		MacroWorldPlan plan(p_params.worldSize[0], p_params.worldSize[1], p_runSeed);
		landmass(plan, p_params, p_runSeed);
		heightRivers(plan, p_params, p_runSeed);
		cities(plan, p_params, p_runSeed);
		biomes(plan, p_params, p_runSeed);
		satellites(plan, p_params, p_runSeed);
		transportGraph(plan, p_params, p_runSeed);
		roads(plan, p_params, p_runSeed);
		return plan;
	}

	void MacroWorldGenerator::landmass(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		auto random = stageRandom(p_runSeed, 1);
		const std::vector<spk::PoissonDisk::Point> sites = spk::PoissonDisk::generateApproximateCount(
			{static_cast<float>(p_plan.width), static_cast<float>(p_plan.height)},
			static_cast<std::size_t>(p_params.landmass.regionCount),
			0.72f,
			40,
			static_cast<std::uint32_t>(random()));
		if (sites.size() < 16)
		{
			throw std::runtime_error("world generation produced too few macro regions");
		}

		std::vector<PlanCell> siteCells;
		siteCells.reserve(sites.size());
		for (const auto &site : sites)
		{
			siteCells.push_back({std::clamp(static_cast<int>(site.x), 0, p_plan.width - 1), std::clamp(static_cast<int>(site.y), 0, p_plan.height - 1)});
		}
		std::vector<std::int32_t> labels(p_plan.landMask.size(), -1);
		for (std::size_t site = 0; site < siteCells.size(); ++site)
		{
			labels[p_plan.index(siteCells[site])] = static_cast<std::int32_t>(site);
		}
		int jump = 1;
		while (jump < std::max(p_plan.width, p_plan.height))
		{
			jump *= 2;
		}
		for (jump /= 2; jump >= 1; jump /= 2)
		{
			std::vector<std::int32_t> next = labels;
			for (int y = 0; y < p_plan.height; ++y)
			{
				for (int x = 0; x < p_plan.width; ++x)
				{
					const std::size_t index = p_plan.index({x, y});
					std::int32_t bestSite = labels[index];
					float bestDistance = bestSite < 0 ? std::numeric_limits<float>::infinity()
													  : distanceSquared({x, y}, siteCells[static_cast<std::size_t>(bestSite)]);
					for (int dy : {-jump, 0, jump})
					{
						for (int dx : {-jump, 0, jump})
						{
							const PlanCell sample{x + dx, y + dy};
							if (!p_plan.contains(sample))
							{
								continue;
							}
							const std::int32_t candidate = labels[p_plan.index(sample)];
							if (candidate < 0)
							{
								continue;
							}
							const float candidateDistance = distanceSquared({x, y}, siteCells[static_cast<std::size_t>(candidate)]);
							if (candidateDistance < bestDistance ||
								(candidateDistance == bestDistance && candidate < bestSite))
							{
								bestDistance = candidateDistance;
								bestSite = candidate;
							}
						}
					}
					next[index] = bestSite;
				}
			}
			labels.swap(next);
		}
		p_plan.macroRegionCells = labels;

		std::vector<std::size_t> regionAreas(sites.size(), 0);
		std::vector<bool> borderRegion(sites.size(), false);
		std::vector<std::set<int>> neighbourSets(sites.size());
		const int border = p_params.landmass.borderOceanWidth;
		for (int y = 0; y < p_plan.height; ++y)
		{
			for (int x = 0; x < p_plan.width; ++x)
			{
				const int region = labels[p_plan.index({x, y})];
				if (region < 0)
				{
					continue;
				}
				++regionAreas[static_cast<std::size_t>(region)];
				if (x < border || y < border || x >= p_plan.width - border || y >= p_plan.height - border)
				{
					borderRegion[static_cast<std::size_t>(region)] = true;
				}
				for (const PlanCell offset : std::array<PlanCell, 2>{{{1, 0}, {0, 1}}})
				{
					const PlanCell adjacent{x + offset.x, y + offset.y};
					if (!p_plan.contains(adjacent))
					{
						continue;
					}
					const int other = labels[p_plan.index(adjacent)];
					if (other >= 0 && other != region)
					{
						neighbourSets[static_cast<std::size_t>(region)].insert(other);
						neighbourSets[static_cast<std::size_t>(other)].insert(region);
					}
				}
			}
		}
		std::vector<std::vector<int>> neighbours(sites.size());
		for (std::size_t region = 0; region < sites.size(); ++region)
		{
			neighbours[region].assign(neighbourSets[region].begin(), neighbourSets[region].end());
		}

		std::uniform_int_distribution<int> landmassCountDistribution(
			p_params.landmass.landmassCount[0], p_params.landmass.landmassCount[1]);
		const int requestedLandmasses = std::min<int>(
			landmassCountDistribution(random), static_cast<int>(sites.size()));
		std::vector<int> candidates;
		for (std::size_t region = 0; region < sites.size(); ++region)
		{
			if (!borderRegion[region] && regionAreas[region] > 0)
			{
				candidates.push_back(static_cast<int>(region));
			}
		}
		if (candidates.size() < static_cast<std::size_t>(requestedLandmasses))
		{
			throw std::runtime_error("not enough interior macro regions for requested landmasses");
		}
		std::ranges::shuffle(candidates, random);
		std::vector<int> landSeeds{candidates.front()};
		while (landSeeds.size() < static_cast<std::size_t>(requestedLandmasses))
		{
			float bestSeparation = -1.0f;
			int best = -1;
			for (const int candidate : candidates)
			{
				if (std::ranges::find(landSeeds, candidate) != landSeeds.end())
				{
					continue;
				}
				float separation = std::numeric_limits<float>::infinity();
				for (const int seed : landSeeds)
				{
					separation = std::min(separation, distanceSquared(siteCells[static_cast<std::size_t>(candidate)], siteCells[static_cast<std::size_t>(seed)]));
				}
				if (separation > bestSeparation)
				{
					bestSeparation = separation;
					best = candidate;
				}
			}
			landSeeds.push_back(best);
		}

		struct Frontier
		{
			float score = 0.0f;
			int region = -1;
			int owner = -1;
			int depth = 0;
			bool operator>(const Frontier &p_other) const noexcept
			{
				if (score != p_other.score)
				{
					return score > p_other.score;
				}
				if (owner != p_other.owner)
				{
					return owner > p_other.owner;
				}
				return region > p_other.region;
			}
		};
		std::vector<int> owner(sites.size(), -1);
		std::priority_queue<Frontier, std::vector<Frontier>, std::greater<>> frontier;
		std::size_t claimedArea = 0;
		const auto pushNeighbours = [&](int p_region, int p_owner, int p_depth, auto &p_frontier) {
			for (const int adjacent : neighbours[static_cast<std::size_t>(p_region)])
			{
				if (owner[static_cast<std::size_t>(adjacent)] >= 0 || borderRegion[static_cast<std::size_t>(adjacent)])
				{
					continue;
				}
				const float spacing = std::sqrt(distanceSquared(
					siteCells[static_cast<std::size_t>(adjacent)], siteCells[static_cast<std::size_t>(landSeeds[static_cast<std::size_t>(p_owner)])]));
				const float jitter = static_cast<float>(splitMix64(p_runSeed ^ static_cast<std::uint64_t>(adjacent * 131 + p_owner * 977)) & 0xffffULL) / 65535.0f;
				p_frontier.push({static_cast<float>(p_depth) + p_params.landmass.compactness * spacing / 16.0f + jitter * 1.5f, adjacent, p_owner, p_depth});
			}
		};
		for (std::size_t landmass = 0; landmass < landSeeds.size(); ++landmass)
		{
			const int seed = landSeeds[landmass];
			owner[static_cast<std::size_t>(seed)] = static_cast<int>(landmass);
			claimedArea += regionAreas[static_cast<std::size_t>(seed)];
			pushNeighbours(seed, static_cast<int>(landmass), 1, frontier);
		}
		const std::size_t targetArea = static_cast<std::size_t>(
			p_params.landmass.landRatio * static_cast<float>(p_plan.landMask.size()));
		while (claimedArea < targetArea && !frontier.empty())
		{
			const Frontier candidate = frontier.top();
			frontier.pop();
			if (owner[static_cast<std::size_t>(candidate.region)] >= 0)
			{
				continue;
			}
			bool touchesOther = false;
			for (const int adjacent : neighbours[static_cast<std::size_t>(candidate.region)])
			{
				const int adjacentOwner = owner[static_cast<std::size_t>(adjacent)];
				if (adjacentOwner >= 0 && adjacentOwner != candidate.owner)
				{
					touchesOther = true;
					break;
				}
			}
			if (touchesOther)
			{
				continue;
			}
			owner[static_cast<std::size_t>(candidate.region)] = candidate.owner;
			claimedArea += regionAreas[static_cast<std::size_t>(candidate.region)];
			pushNeighbours(candidate.region, candidate.owner, candidate.depth + 1, frontier);
		}

		std::vector<std::uint8_t> macroLand(p_plan.landMask.size(), 0);
		for (std::size_t index = 0; index < labels.size(); ++index)
		{
			if (labels[index] >= 0 && owner[static_cast<std::size_t>(labels[index])] >= 0)
			{
				macroLand[index] = 1;
			}
		}
		std::vector<std::uint16_t> boundaryDistance(labels.size(), std::numeric_limits<std::uint16_t>::max());
		std::queue<std::size_t> boundaryQueue;
		for (int y = 0; y < p_plan.height; ++y)
		{
			for (int x = 0; x < p_plan.width; ++x)
			{
				const PlanCell cell{x, y};
				const std::size_t index = p_plan.index(cell);
				for (const PlanCell offset : CardinalNeighbours)
				{
					const PlanCell adjacent{x + offset.x, y + offset.y};
					if (!p_plan.contains(adjacent) || macroLand[index] != macroLand[p_plan.index(adjacent)])
					{
						boundaryDistance[index] = 0;
						boundaryQueue.push(index);
						break;
					}
				}
			}
		}
		while (!boundaryQueue.empty())
		{
			const std::size_t index = boundaryQueue.front();
			boundaryQueue.pop();
			const PlanCell cell{static_cast<int>(index % static_cast<std::size_t>(p_plan.width)), static_cast<int>(index / static_cast<std::size_t>(p_plan.width))};
			for (const PlanCell offset : CardinalNeighbours)
			{
				const PlanCell adjacent{cell.x + offset.x, cell.y + offset.y};
				if (!p_plan.contains(adjacent))
				{
					continue;
				}
				const std::size_t adjacentIndex = p_plan.index(adjacent);
				const auto distance = static_cast<std::uint16_t>(boundaryDistance[index] + 1);
				if (distance >= boundaryDistance[adjacentIndex])
				{
					continue;
				}
				boundaryDistance[adjacentIndex] = distance;
				boundaryQueue.push(adjacentIndex);
			}
		}

		spk::Perlin broad({.seed = static_cast<std::uint32_t>(random()), .octaves = 4, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = p_params.landmass.noiseFrequency});
		spk::Perlin detail({.seed = static_cast<std::uint32_t>(random()), .octaves = 2, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = p_params.landmass.detailFrequency});
		std::vector<float> continentalness(labels.size(), 0.0f);
		std::vector<float> eligibleScores;
		for (int y = border; y < p_plan.height - border; ++y)
		{
			for (int x = border; x < p_plan.width - border; ++x)
			{
				const std::size_t index = p_plan.index({x, y});
				const float signedDistance = macroLand[index] ? static_cast<float>(boundaryDistance[index]) : -static_cast<float>(boundaryDistance[index]);
				continentalness[index] = signedDistance + broad.raw2D(static_cast<float>(x), static_cast<float>(y)) * p_params.landmass.noiseAmplitude * 42.0f + detail.raw2D(static_cast<float>(x), static_cast<float>(y)) * p_params.landmass.detailAmplitude * 24.0f - p_params.landmass.seaLevel * 20.0f;
				eligibleScores.push_back(continentalness[index]);
			}
		}
		if (eligibleScores.size() < targetArea)
		{
			throw std::runtime_error("land ratio exceeds usable world area");
		}
		std::nth_element(
			eligibleScores.begin(), eligibleScores.begin() + static_cast<std::ptrdiff_t>(targetArea), eligibleScores.end(), std::greater<>());
		const float seaThreshold = eligibleScores[targetArea];
		std::ranges::fill(p_plan.landMask, 0);
		for (int y = border; y < p_plan.height - border; ++y)
		{
			for (int x = border; x < p_plan.width - border; ++x)
			{
				const std::size_t index = p_plan.index({x, y});
				p_plan.landMask[index] = continentalness[index] >= seaThreshold ? 1 : 0;
			}
		}
		retainMajorLandmassesAndBuildOcean(p_plan, requestedLandmasses);
	}

	void MacroWorldGenerator::heightRivers(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		auto random = stageRandom(p_runSeed, 2);
		spk::Perlin base({.seed = static_cast<std::uint32_t>(random()), .octaves = 4, .persistence = 0.52f, .lacunarity = 2.0f, .frequency = p_params.height.baseFrequency});
		spk::Perlin mountain({.seed = static_cast<std::uint32_t>(random()), .octaves = 3, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = p_params.height.mountainFrequency});
		for (int y = 0; y < p_plan.height; ++y)
		{
			for (int x = 0; x < p_plan.width; ++x)
			{
				const std::size_t index = p_plan.index({x, y});
				if (!p_plan.landMask[index])
				{
					continue;
				}
				const float low = base.sample2D(static_cast<float>(x), static_cast<float>(y));
				const float high = mountain.sample2D(static_cast<float>(x), static_cast<float>(y));
				const float mountainPart = std::max(0.0f, (high - p_params.height.mountainThreshold) / std::max(0.001f, 1.0f - p_params.height.mountainThreshold));
				p_plan.heights[index] = 1.0f + (low * 0.42f + mountainPart * 0.58f) * static_cast<float>(p_params.height.maxHeight - 1);
			}
		}

		std::vector<float> smoothed = p_plan.heights;
		for (int pass = 0; pass < 2; ++pass)
		{
			for (int y = 1; y + 1 < p_plan.height; ++y)
			{
				for (int x = 1; x + 1 < p_plan.width; ++x)
				{
					const PlanCell cell{x, y};
					const std::size_t index = p_plan.index(cell);
					if (!p_plan.landMask[index])
					{
						continue;
					}
					float sum = p_plan.heights[index] * 4.0f;
					float weight = 4.0f;
					for (const PlanCell offset : std::array<PlanCell, 4>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}})
					{
						const std::size_t neighbour = p_plan.index({x + offset.x, y + offset.y});
						if (p_plan.landMask[neighbour])
						{
							sum += p_plan.heights[neighbour];
							weight += 1.0f;
						}
					}
					smoothed[index] = sum / weight;
				}
			}
			p_plan.heights.swap(smoothed);
			smoothed = p_plan.heights;
		}

		float minimumLandHeight = std::numeric_limits<float>::max();
		float maximumLandHeight = std::numeric_limits<float>::lowest();
		for (std::size_t index = 0; index < p_plan.heights.size(); ++index)
		{
			if (!p_plan.landMask[index])
			{
				continue;
			}
			minimumLandHeight = std::min(minimumLandHeight, p_plan.heights[index]);
			maximumLandHeight = std::max(maximumLandHeight, p_plan.heights[index]);
		}
		const float landHeightRange = maximumLandHeight - minimumLandHeight;
		if (landHeightRange <= 0.0001f)
		{
			throw std::runtime_error("world generation produced a flat height field");
		}
		for (std::size_t index = 0; index < p_plan.heights.size(); ++index)
		{
			if (!p_plan.landMask[index])
			{
				continue;
			}
			const float normalized = std::clamp(
				(p_plan.heights[index] - minimumLandHeight) / landHeightRange, 0.0f, 1.0f);
			p_plan.heights[index] = 1.0f + std::pow(normalized, 1.15f) *
											   static_cast<float>(p_params.height.maxHeight - 1);
		}

		std::vector<PlanCell> sources;
		for (int y = 2; y + 2 < p_plan.height; y += 3)
		{
			for (int x = 2; x + 2 < p_plan.width; x += 3)
			{
				if (p_plan.landMask[p_plan.index({x, y})] && p_plan.heights[p_plan.index({x, y})] > p_params.height.maxHeight * 0.52f)
				{
					sources.push_back({x, y});
				}
			}
		}
		std::ranges::sort(sources, [&](PlanCell p_a, PlanCell p_b) {
			return p_plan.heights[p_plan.index(p_a)] > p_plan.heights[p_plan.index(p_b)];
		});

		const std::size_t targetRiverCount = std::min<std::size_t>(static_cast<std::size_t>(p_params.cities.majorCount), sources.size());
		for (const PlanCell source : sources)
		{
			if (p_plan.rivers.size() >= targetRiverCount)
			{
				break;
			}
			bool nearExisting = false;
			for (const PlanRiver &river : p_plan.rivers)
			{
				nearExisting = nearExisting || (!river.cells.empty() && distanceSquared(source, river.cells.front()) < 32.0f * 32.0f);
			}
			if (nearExisting)
			{
				continue;
			}
			PlanRiver river;
			PlanCell current = source;
			std::unordered_set<std::size_t> visited;
			const int maxSteps = (p_plan.width + p_plan.height) * 2;
			for (int step = 0; step < maxSteps; ++step)
			{
				const std::size_t currentIndex = p_plan.index(current);
				if (!visited.insert(currentIndex).second)
				{
					river.endsInLakeBasin = true;
					break;
				}
				river.cells.push_back(current);
				p_plan.riverMask[currentIndex] = 1;
				std::vector<PlanCell> downhill;
				float bestHeight = p_plan.heights[currentIndex];
				bool foundSea = false;
				for (const PlanCell offset : Neighbours)
				{
					const PlanCell next{current.x + offset.x, current.y + offset.y};
					if (!p_plan.contains(next))
					{
						foundSea = true;
						break;
					}
					const std::size_t nextIndex = p_plan.index(next);
					if (!p_plan.landMask[nextIndex])
					{
						foundSea = true;
						break;
					}
					const float candidateHeight = p_plan.heights[nextIndex];
					if (candidateHeight < bestHeight - 0.001f)
					{
						bestHeight = candidateHeight;
						downhill.clear();
						downhill.push_back(next);
					}
					else if (std::abs(candidateHeight - bestHeight) < 0.001f)
					{
						downhill.push_back(next);
					}
				}
				if (foundSea)
				{
					river.reachesSea = true;
					break;
				}
				if (downhill.empty())
				{
					river.endsInLakeBasin = true;
					break;
				}
				std::uniform_int_distribution<std::size_t> pick(0, downhill.size() - 1);
				current = downhill[pick(random)];
			}
			if (!river.reachesSea && !river.endsInLakeBasin)
			{
				river.endsInLakeBasin = true;
			}
			p_plan.rivers.push_back(std::move(river));
		}
	}

	void MacroWorldGenerator::cities(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		auto random = stageRandom(p_runSeed, 3);
		const float flatness = std::max(1.5f, static_cast<float>(p_params.height.maxHeight) * 0.09f);
		spk::PoissonDisk::Parameters poisson;
		poisson.size = {static_cast<float>(p_plan.width), static_cast<float>(p_plan.height)};
		poisson.radius = p_params.cities.minSpacing;
		poisson.triesPerPoint = 60;
		poisson.seed = static_cast<std::uint32_t>(random());
		poisson.accept = [&](const spk::PoissonDisk::Point &p_point) {
			return isFlatLand(p_plan, {static_cast<int>(p_point.x), static_cast<int>(p_point.y)}, flatness);
		};
		std::vector<spk::PoissonDisk::Point> candidates = spk::PoissonDisk::generate(poisson);
		std::ranges::stable_sort(candidates, [&](const auto &p_a, const auto &p_b) {
			const int coastA = waterDistance(p_plan, {static_cast<int>(p_a.x), static_cast<int>(p_a.y)}, 24);
			const int coastB = waterDistance(p_plan, {static_cast<int>(p_b.x), static_cast<int>(p_b.y)}, 24);
			const float ideal = 5.0f + (1.0f - p_params.cities.coastalBias) * 19.0f;
			return std::abs(static_cast<float>(coastA) - ideal) < std::abs(static_cast<float>(coastB) - ideal);
		});

		for (const auto &point : candidates)
		{
			if (p_plan.cities.size() >= static_cast<std::size_t>(p_params.cities.majorCount))
			{
				break;
			}
			p_plan.cities.push_back({{static_cast<int>(point.x), static_cast<int>(point.y)}, {}, 1.0f});
		}
		if (p_plan.cities.size() != static_cast<std::size_t>(p_params.cities.majorCount))
		{
			throw std::runtime_error("world generation could not place the requested major cities; reduce minSpacing or adjust landmass parameters");
		}
	}

	void MacroWorldGenerator::biomes(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		auto random = stageRandom(p_runSeed, 4);
		const std::uint16_t unreachable = std::numeric_limits<std::uint16_t>::max();
		std::vector<std::uint16_t> riverDistances(p_plan.riverMask.size(), unreachable);
		std::queue<std::size_t> pending;
		for (std::size_t index = 0; index < p_plan.riverMask.size(); ++index)
		{
			if (!p_plan.riverMask[index])
			{
				continue;
			}
			riverDistances[index] = 0;
			pending.push(index);
		}
		while (!pending.empty())
		{
			const std::size_t index = pending.front();
			pending.pop();
			const PlanCell cell{
				static_cast<int>(index % static_cast<std::size_t>(p_plan.width)),
				static_cast<int>(index / static_cast<std::size_t>(p_plan.width))};
			for (const PlanCell offset : CardinalNeighbours)
			{
				const PlanCell neighbour{cell.x + offset.x, cell.y + offset.y};
				if (!p_plan.contains(neighbour))
				{
					continue;
				}
				const std::size_t neighbourIndex = p_plan.index(neighbour);
				const auto candidate = static_cast<std::uint16_t>(
					std::min<int>(unreachable - 1, static_cast<int>(riverDistances[index]) + 1));
				if (candidate >= riverDistances[neighbourIndex])
				{
					continue;
				}
				riverDistances[neighbourIndex] = candidate;
				pending.push(neighbourIndex);
			}
		}
		spk::Perlin temperatureNoise({.seed = static_cast<std::uint32_t>(random()), .octaves = 3, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 0.006f});
		spk::Perlin moistureNoise({.seed = static_cast<std::uint32_t>(random()), .octaves = 3, .persistence = 0.52f, .lacunarity = 2.0f, .frequency = 0.0075f});

		const auto suitability = [&](const std::string &p_biome, PlanCell p_cell) {
			const std::size_t index = p_plan.index(p_cell);
			const float elevation = std::clamp(
				p_plan.heights[index] / static_cast<float>(p_params.height.maxHeight), 0.0f, 1.0f);
			const float inland = std::clamp(static_cast<float>(p_plan.oceanDistances[index]) / 80.0f, 0.0f, 1.0f);
			const float coastal = 1.0f - inland;
			const float riverDistance = riverDistances[index] == unreachable ? 80.0f : static_cast<float>(riverDistances[index]);
			const float riverNear = 1.0f - std::clamp(riverDistance / 48.0f, 0.0f, 1.0f);
			const float latitude = static_cast<float>(p_cell.y) / static_cast<float>(std::max(1, p_plan.height - 1));
			const float equatorWarmth = 1.0f - std::abs(latitude * 2.0f - 1.0f);
			const float temperature = std::clamp(
				0.12f + equatorWarmth * 0.78f + temperatureNoise.raw2D(static_cast<float>(p_cell.x), static_cast<float>(p_cell.y)) * 0.2f - elevation * 0.42f,
				0.0f,
				1.0f);
			const float moisture = std::clamp(
				0.12f + coastal * 0.38f + riverNear * 0.3f + moistureNoise.raw2D(static_cast<float>(p_cell.x), static_cast<float>(p_cell.y)) * 0.28f,
				0.0f,
				1.0f);
			if (p_biome == "coast")
			{
				return 2.2f * coastal + 0.2f * (1.0f - elevation);
			}
			if (p_biome == "swamp")
			{
				return moisture > 0.68f && std::max(coastal, riverNear) > 0.45f
						   ? 2.1f + moisture - elevation
						   : -2.0f;
			}
			if (p_biome == "volcano")
			{
				return elevation > 0.82f ? 2.3f + elevation + temperature * 0.2f : -3.0f;
			}
			if (p_biome == "highland")
			{
				return elevation > 0.62f ? 1.9f + elevation + inland * 0.2f : -1.5f;
			}
			if (p_biome == "tundra")
			{
				return temperature < 0.34f ? 2.0f + (0.34f - temperature) + elevation * 0.3f : -1.0f;
			}
			if (p_biome == "desert")
			{
				return temperature > 0.58f && moisture < 0.42f
						   ? 2.0f + temperature - moisture + inland * 0.3f
						   : -1.0f;
			}
			if (p_biome == "forest")
			{
				return moisture > 0.48f ? 1.25f + moisture - std::abs(temperature - 0.55f) * 0.3f : 0.2f;
			}
			if (p_biome == "meadow")
			{
				return 1.0f + (1.0f - std::abs(temperature - 0.58f)) * 0.25f +
					   (1.0f - std::abs(moisture - 0.45f)) * 0.25f;
			}
			return 0.5f;
		};

		p_plan.biomeIds = p_params.biomes;
		std::uniform_real_distribution<float> weight(0.82f, 1.18f);
		for (PlanCity &city : p_plan.cities)
		{
			city.biomeWeight = weight(random);
		}
		const auto coastBiome = std::ranges::find(p_plan.biomeIds, std::string("coast"));
		const std::size_t coastIndex = coastBiome == p_plan.biomeIds.end()
										   ? p_plan.biomeIds.size()
										   : static_cast<std::size_t>(std::distance(p_plan.biomeIds.begin(), coastBiome));
		const auto firstInteriorBiome = std::ranges::find_if(p_plan.biomeIds, [](const std::string &p_biome) {
			return p_biome != "coast";
		});
		if (firstInteriorBiome == p_plan.biomeIds.end())
		{
			throw std::runtime_error("world generation requires at least one non-coast biome");
		}
		const std::size_t fallbackBiome = static_cast<std::size_t>(
			std::distance(p_plan.biomeIds.begin(), firstInteriorBiome));
		spk::Perlin borders({.seed = static_cast<std::uint32_t>(random()), .octaves = 2, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 0.025f});
		for (int y = 0; y < p_plan.height; ++y)
		{
			for (int x = 0; x < p_plan.width; ++x)
			{
				const PlanCell cell{x, y};
				const std::size_t index = p_plan.index(cell);
				if (!p_plan.landMask[index])
				{
					continue;
				}
				float best = -std::numeric_limits<float>::infinity();
				std::size_t bestBiome = fallbackBiome;
				const float noise = borders.raw2D(static_cast<float>(x), static_cast<float>(y)) * 10.0f;
				const int coastThreshold = 7 + static_cast<int>(std::round(noise * 0.2f));
				if (coastIndex < p_plan.biomeIds.size() && p_plan.oceanDistances[index] <= coastThreshold)
				{
					p_plan.biomeCells[index] = static_cast<std::int16_t>(coastIndex);
					continue;
				}
				for (std::size_t biome = 0; biome < p_plan.biomeIds.size(); ++biome)
				{
					if (biome == coastIndex)
					{
						continue;
					}
					const int borderBand = static_cast<int>(biome % 3) - 1;
					const float score = suitability(p_plan.biomeIds[biome], cell) +
										noise * static_cast<float>(borderBand) * 0.018f;
					if (score > best)
					{
						best = score;
						bestBiome = biome;
					}
				}
				p_plan.biomeCells[index] = static_cast<std::int16_t>(bestBiome);
			}
		}

		for (int pass = 0; pass < 2; ++pass)
		{
			std::vector<std::int16_t> smoothed = p_plan.biomeCells;
			for (int y = 1; y + 1 < p_plan.height; ++y)
			{
				for (int x = 1; x + 1 < p_plan.width; ++x)
				{
					const std::size_t index = p_plan.index({x, y});
					if (!p_plan.landMask[index] || p_plan.oceanDistances[index] <= 9)
					{
						continue;
					}
					std::vector<int> counts(p_plan.biomeIds.size(), 0);
					for (const PlanCell offset : CardinalNeighbours)
					{
						const std::int16_t neighbour = p_plan.biomeCells[p_plan.index({x + offset.x, y + offset.y})];
						if (neighbour >= 0 && static_cast<std::size_t>(neighbour) < counts.size())
						{
							++counts[static_cast<std::size_t>(neighbour)];
						}
					}
					const auto majority = std::ranges::max_element(counts);
					if (majority != counts.end() && *majority >= 3)
					{
						smoothed[index] = static_cast<std::int16_t>(std::distance(counts.begin(), majority));
					}
				}
			}
			p_plan.biomeCells.swap(smoothed);
		}
		for (PlanCity &city : p_plan.cities)
		{
			city.biome = p_plan.queryCell(city.cell).biome;
		}
	}

	void MacroWorldGenerator::satellites(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		auto random = stageRandom(p_runSeed, 5);
		std::uniform_int_distribution<int> countDistribution(p_params.cities.satellitesPerCity[0], p_params.cities.satellitesPerCity[1]);
		std::uniform_real_distribution<float> angleDistribution(0.0f, 6.283185307f);
		std::uniform_real_distribution<float> radiusDistribution(p_params.cities.satelliteRadius * 0.28f, p_params.cities.satelliteRadius);
		const float flatness = std::max(2.0f, static_cast<float>(p_params.height.maxHeight) * 0.12f);
		for (std::size_t city = 0; city < p_plan.cities.size(); ++city)
		{
			const int target = countDistribution(random);
			int created = 0;
			for (int attempt = 0; attempt < 3000 && created < target; ++attempt)
			{
				const float angle = angleDistribution(random);
				const float radius = radiusDistribution(random);
				const PlanCell candidate{
					p_plan.cities[city].cell.x + static_cast<int>(std::round(std::cos(angle) * radius)),
					p_plan.cities[city].cell.y + static_cast<int>(std::round(std::sin(angle) * radius))};
				if (!isFlatLand(p_plan, candidate, flatness))
				{
					continue;
				}
				bool tooClose = distanceSquared(candidate, p_plan.cities[city].cell) < 12.0f * 12.0f;
				for (const PlanSettlement &settlement : p_plan.settlements)
				{
					tooClose = tooClose || distanceSquared(candidate, settlement.cell) < 10.0f * 10.0f;
				}
				if (tooClose)
				{
					continue;
				}
				p_plan.settlements.push_back({candidate, city, p_plan.cities[city].biome});
				++created;
			}
			if (created != target)
			{
				throw std::runtime_error("world generation could not place all satellite settlements");
			}
		}
	}

	void MacroWorldGenerator::transportGraph(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		(void)stageRandom(p_runSeed, 6);
		p_plan.edges.clear();
		const std::size_t cityCount = p_plan.cities.size();
		std::vector<bool> connected(cityCount, false);
		connected[0] = true;
		for (std::size_t count = 1; count < cityCount; ++count)
		{
			float best = std::numeric_limits<float>::infinity();
			std::size_t from = 0;
			std::size_t to = 0;
			for (std::size_t a = 0; a < cityCount; ++a)
			{
				for (std::size_t b = 0; b < cityCount; ++b)
				{
					if (connected[a] && !connected[b] && distanceSquared(p_plan.cities[a].cell, p_plan.cities[b].cell) < best)
					{
						best = distanceSquared(p_plan.cities[a].cell, p_plan.cities[b].cell);
						from = a;
						to = b;
					}
				}
			}
			connected[to] = true;
			const auto direct = rasterLine(p_plan.cities[from].cell, p_plan.cities[to].cell);
			p_plan.edges.push_back({from, to, classifyPath(p_plan, p_params, direct)});
		}

		struct Candidate
		{
			float distance;
			std::size_t from;
			std::size_t to;
		};
		std::vector<Candidate> extras;
		for (std::size_t a = 0; a < cityCount; ++a)
		{
			for (std::size_t b = a + 1; b < cityCount; ++b)
			{
				if (!edgeExists(p_plan.edges, a, b))
				{
					extras.push_back({distanceSquared(p_plan.cities[a].cell, p_plan.cities[b].cell), a, b});
				}
			}
		}
		std::ranges::sort(extras, {}, &Candidate::distance);
		for (int i = 0; i < p_params.transport.extraEdges && i < static_cast<int>(extras.size()); ++i)
		{
			const Candidate candidate = extras[static_cast<std::size_t>(i)];
			p_plan.edges.push_back({candidate.from, candidate.to, classifyPath(p_plan, p_params, rasterLine(p_plan.nodeCell(candidate.from), p_plan.nodeCell(candidate.to)))});
		}

		for (std::size_t settlement = 0; settlement < p_plan.settlements.size(); ++settlement)
		{
			const std::size_t node = cityCount + settlement;
			const std::size_t hub = p_plan.settlements[settlement].hubCity;
			p_plan.edges.push_back({node, hub, classifyPath(p_plan, p_params, rasterLine(p_plan.nodeCell(node), p_plan.nodeCell(hub)))});
		}
	}

	void MacroWorldGenerator::roads(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed)
	{
		(void)stageRandom(p_runSeed, 7);
		std::ranges::fill(p_plan.roadMask, 0);
		std::ranges::fill(p_plan.featureMask, 0);
		p_plan.roads.clear();
		p_plan.features.clear();
		for (std::size_t edgeIndex = 0; edgeIndex < p_plan.edges.size(); ++edgeIndex)
		{
			PlanTransportEdge &edge = p_plan.edges[edgeIndex];
			const PlanCell from = p_plan.nodeCell(edge.from);
			const PlanCell to = p_plan.nodeCell(edge.to);
			std::vector<PlanCell> path = findPath(p_plan, p_params, from, to, edge.classification == TransportClass::Sea);
			if (path.empty())
			{
				path = findPath(p_plan, p_params, from, to, true);
			}
			if (path.empty())
			{
				throw std::runtime_error("world generation could not route a transport edge");
			}
			edge.classification = classifyPath(p_plan, p_params, path);
			for (const PlanCell cell : path)
			{
				p_plan.roadMask[p_plan.index(cell)] = 1;
			}
			p_plan.roads.push_back({edgeIndex, path});

			if (edge.classification == TransportClass::Bridge)
			{
				for (const PlanCell cell : path)
				{
					if (p_plan.riverMask[p_plan.index(cell)])
					{
						addFeature(p_plan, PlanFeatureType::Bridge, cell, edgeIndex);
					}
				}
			}
			else if (edge.classification == TransportClass::Sea)
			{
				for (std::size_t i = 1; i < path.size(); ++i)
				{
					const bool previousLand = p_plan.landMask[p_plan.index(path[i - 1])] != 0;
					const bool currentLand = p_plan.landMask[p_plan.index(path[i])] != 0;
					if (previousLand != currentLand)
					{
						addFeature(p_plan, PlanFeatureType::Port, previousLand ? path[i - 1] : path[i], edgeIndex);
					}
				}
			}
			else if (edge.classification == TransportClass::Tunnel)
			{
				bool placed = false;
				for (std::size_t i = 1; i < path.size(); ++i)
				{
					const float rise = std::abs(p_plan.heights[p_plan.index(path[i])] - p_plan.heights[p_plan.index(path[i - 1])]);
					if (rise > 1.5f)
					{
						addFeature(p_plan, PlanFeatureType::Tunnel, path[i], edgeIndex);
						placed = true;
					}
				}
				if (!placed)
				{
					addFeature(p_plan, PlanFeatureType::Tunnel, path[path.size() / 2], edgeIndex);
				}
			}
		}
	}
}
