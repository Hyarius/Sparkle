#include "world/generator/world_plan.hpp"

#include "core/registry.hpp"
#include "world/biome_definition.hpp"
#include "world/generator/placement_rules.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

// Port of the topology-first Python prototype (worldgen-2.py): the same stages run in
// the same order with the same tunables, but on plain C++ grids. Randomness is drawn
// from generators seeded by hash(masterSeed, semanticPath) so adding a stage never
// reshuffles the output of existing stages. Outputs are not bit-identical to the
// Python previewer (different RNG engine), only structurally equivalent.

namespace pg
{
	namespace
	{
		struct Cell
		{
			int row = 0;
			int col = 0;

			[[nodiscard]] bool operator==(const Cell &) const noexcept = default;
		};

		[[nodiscard]] std::uint64_t deriveSeed(std::uint64_t p_master, const std::string &p_path)
		{
			// FNV-1a over "<master>::<path>"; stable across platforms and runs.
			const std::string key = std::to_string(p_master) + "::" + p_path;
			std::uint64_t hash = 1469598103934665603ULL;
			for (const char character : key)
			{
				hash ^= static_cast<std::uint8_t>(character);
				hash *= 1099511628211ULL;
			}
			return hash;
		}

		struct Rng
		{
			std::mt19937_64 engine;

			explicit Rng(std::uint64_t p_seed) : engine(p_seed) {}

			[[nodiscard]] double uniform()
			{
				return std::uniform_real_distribution<double>(0.0, 1.0)(engine);
			}
			[[nodiscard]] int below(int p_count)
			{
				return std::uniform_int_distribution<int>(0, p_count - 1)(engine);
			}
			template <typename TValue>
			void shuffle(std::vector<TValue> &p_values)
			{
				std::shuffle(p_values.begin(), p_values.end(), engine);
			}
		};

		using Mask = PlanGrid<std::uint8_t>;
		using Field = PlanGrid<double>;

		constexpr double INF = 1e18;

		// Neighbor order matches the Python _neighbors4 (north, south, west, east) so BFS
		// tie-breaks resolve the same way.
		template <typename TVisitor>
		void forNeighbors4(int p_row, int p_col, int p_size, TVisitor &&p_visit)
		{
			if (p_row > 0)
			{
				p_visit(p_row - 1, p_col);
			}
			if (p_row < p_size - 1)
			{
				p_visit(p_row + 1, p_col);
			}
			if (p_col > 0)
			{
				p_visit(p_row, p_col - 1);
			}
			if (p_col < p_size - 1)
			{
				p_visit(p_row, p_col + 1);
			}
		}

		// Cheap smooth value noise in [0,1]: bilinear over a random lattice with a
		// smoothstep on the fractional part (port of _value_noise).
		[[nodiscard]] Field valueNoise(Rng &p_rng, int p_size, double p_scale)
		{
			const int n = std::max(2, static_cast<int>(p_size / p_scale));
			std::vector<double> lattice(static_cast<std::size_t>(n + 2) * (n + 2));
			for (double &value : lattice)
			{
				value = p_rng.uniform();
			}
			const auto latticeAt = [&](int p_row, int p_col) { return lattice[static_cast<std::size_t>(p_row) * (n + 2) + p_col]; };

			Field result(p_size);
			for (int i = 0; i < p_size; ++i)
			{
				const double y = static_cast<double>(i) * n / p_size;
				const int y0 = static_cast<int>(y);
				double fy = y - y0;
				fy = fy * fy * (3.0 - 2.0 * fy);
				for (int j = 0; j < p_size; ++j)
				{
					const double x = static_cast<double>(j) * n / p_size;
					const int x0 = static_cast<int>(x);
					double fx = x - x0;
					fx = fx * fx * (3.0 - 2.0 * fx);
					const double top = latticeAt(y0, x0) * (1.0 - fx) + latticeAt(y0, x0 + 1) * fx;
					const double bottom = latticeAt(y0 + 1, x0) * (1.0 - fx) + latticeAt(y0 + 1, x0 + 1) * fx;
					result.at(i, j) = top * (1.0 - fy) + bottom * fy;
				}
			}
			return result;
		}

		// Multi-source BFS distance (orthogonal steps) to the True cells of the mask.
		[[nodiscard]] PlanGrid<int> distanceTo(const Mask &p_mask)
		{
			const int size = p_mask.size();
			PlanGrid<int> distance(size, std::numeric_limits<int>::max() / 2);
			std::deque<Cell> queue;
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (p_mask.at(i, j) != 0)
					{
						distance.at(i, j) = 0;
						queue.push_back({i, j});
					}
				}
			}
			while (!queue.empty())
			{
				const Cell cell = queue.front();
				queue.pop_front();
				forNeighbors4(cell.row, cell.col, size, [&](int p_row, int p_col) {
					if (distance.at(p_row, p_col) > distance.at(cell.row, cell.col) + 1)
					{
						distance.at(p_row, p_col) = distance.at(cell.row, cell.col) + 1;
						queue.push_back({p_row, p_col});
					}
				});
			}
			return distance;
		}

		// Farthest-point sampling: balanced spread of k picks over the candidate cells.
		[[nodiscard]] std::vector<Cell> farthestPointSeeds(const std::vector<Cell> &p_candidates, int p_count, Rng &p_rng)
		{
			if (p_candidates.empty() || p_count <= 0)
			{
				return {};
			}
			const int total = static_cast<int>(p_candidates.size());
			std::vector<Cell> chosen;
			chosen.push_back(p_candidates[p_rng.below(total)]);
			std::vector<double> best(total);
			for (int index = 0; index < total; ++index)
			{
				const double dy = p_candidates[index].row - chosen[0].row;
				const double dx = p_candidates[index].col - chosen[0].col;
				best[index] = dy * dy + dx * dx;
			}
			while (static_cast<int>(chosen.size()) < std::min(p_count, total))
			{
				int farthest = 0;
				for (int index = 1; index < total; ++index)
				{
					if (best[index] > best[farthest])
					{
						farthest = index;
					}
				}
				chosen.push_back(p_candidates[farthest]);
				for (int index = 0; index < total; ++index)
				{
					const double dy = p_candidates[index].row - p_candidates[farthest].row;
					const double dx = p_candidates[index].col - p_candidates[farthest].col;
					best[index] = std::min(best[index], dy * dy + dx * dx);
				}
			}
			return chosen;
		}

		// Orthogonal connected components; 0 stays "not in mask", 1.. are component ids.
		[[nodiscard]] PlanGrid<int> labelComponents(const Mask &p_mask, int &p_count)
		{
			const int size = p_mask.size();
			PlanGrid<int> component(size, 0);
			p_count = 0;
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (p_mask.at(i, j) == 0 || component.at(i, j) != 0)
					{
						continue;
					}
					++p_count;
					std::deque<Cell> queue{{i, j}};
					component.at(i, j) = p_count;
					while (!queue.empty())
					{
						const Cell cell = queue.front();
						queue.pop_front();
						forNeighbors4(cell.row, cell.col, size, [&](int p_row, int p_col) {
							if (p_mask.at(p_row, p_col) != 0 && component.at(p_row, p_col) == 0)
							{
								component.at(p_row, p_col) = p_count;
								queue.push_back({p_row, p_col});
							}
						});
					}
				}
			}
			return component;
		}

		[[nodiscard]] int countDiagonalOnly(const Mask &p_mask)
		{
			const int size = p_mask.size();
			int bad = 0;
			for (int i = 0; i < size; ++i)
			{
				for (int j = 0; j < size; ++j)
				{
					if (p_mask.at(i, j) == 0)
					{
						continue;
					}
					bool orthogonal = false;
					forNeighbors4(i, j, size, [&](int p_row, int p_col) { orthogonal |= p_mask.at(p_row, p_col) != 0; });
					bool diagonal = false;
					for (const auto &[dr, dc] : {std::pair{-1, -1}, {-1, 1}, {1, -1}, {1, 1}})
					{
						const int row = i + dr;
						const int col = j + dc;
						if (row >= 0 && col >= 0 && row < size && col < size && p_mask.at(row, col) != 0)
						{
							diagonal = true;
						}
					}
					if (diagonal && !orthogonal)
					{
						++bad;
					}
				}
			}
			return bad;
		}

		// ------------------------------------------------------------------
		// The generator: one struct so the stages can share grids without
		// dragging parameter lists around.
		// ------------------------------------------------------------------
		struct Generator
		{
			const WorldGenConfig &cfg;
			const PlanPlacementRules &placementRules;
			WorldPlan plan;
			int size;

			PlanGrid<int> continents{};
			int continentCount = 0;
			std::map<int, int> zoneContinent;
			std::map<std::pair<int, int>, std::vector<Cell>> borders;
			PlanGrid<int> distOcean{};

			Generator(
				const WorldGenConfig &p_config,
				const std::vector<PlanBiome> &p_biomes,
				const PlanPlacementRules &p_placementRules) :
				cfg(p_config), placementRules(p_placementRules), size(p_config.size)
			{
				if (p_biomes.empty())
				{
					throw std::runtime_error(
						"world generation needs at least one biome definition with a \"worldgen\" block");
				}
				plan.config = cfg;
				plan.biomes = p_biomes;
				plan.land = Mask(size, 0);
				plan.zone = PlanGrid<std::int16_t>(size, -1);
				plan.height = PlanGrid<std::int8_t>(size, -1);
				plan.water = Mask(size, 0);
				plan.lake = Mask(size, 0);
				plan.road = Mask(size, 0);
				plan.bridge = Mask(size, 0);
			}

			[[nodiscard]] Rng rngFor(const std::string &p_path) const
			{
				return Rng(deriveSeed(cfg.masterSeed, p_path));
			}

			[[nodiscard]] bool isLand(int p_row, int p_col) const { return plan.land.at(p_row, p_col) != 0; }

			// ---------------- Stage A: world graph ----------------
			void buildWorldGraph()
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
					plan.zones.push_back({.id = index,
										  .biomeIndex = biomeIndices[index % biomeIndices.size()],
										  .progression = order[index]});
				}
			}

			// ---------------- Stage B: landmass + zones ----------------
			void buildLandmass()
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

			void dropTinyIslands()
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

			[[nodiscard]] std::vector<Cell> landCells() const
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

			void assignZones()
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
			void enforceContiguousZones()
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

			void mapZoneContinents()
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

			void findBorders()
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
			void assignHeights()
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
						if (dCoast <= 1.0)
						{
							level = 0; // shores read as sea level
						}
						plan.height.at(i, j) = static_cast<std::int8_t>(level);
					}
				}
			}

			// ---------------- Stage D: hydrology ----------------
			void generateWater()
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
					const auto depthOf = [&](const Cell &p_cell) { return filled.at(p_cell.row, p_cell.col) - effective.at(p_cell.row, p_cell.col); };
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
							std::sort(next.begin(), next.end(), [&](const Cell &p_a, const Cell &p_b) { return depthOf(p_a) > depthOf(p_b); });
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

			// ---------------- Stage C bis: gateways ----------------
			void resolveGateways()
			{
				Rng rng = rngFor("world/gateways");
				for (const auto &[key, cells] : borders)
				{
					if (cells.size() < 2)
					{
						continue;
					}
					double centroidRow = 0.0;
					double centroidCol = 0.0;
					for (const Cell &cell : cells)
					{
						centroidRow += cell.row;
						centroidCol += cell.col;
					}
					centroidRow /= cells.size();
					centroidCol /= cells.size();
					std::size_t nearest = 0;
					std::size_t farthest = 0;
					double nearestDistance = INF;
					double farthestDistance = -1.0;
					for (std::size_t index = 0; index < cells.size(); ++index)
					{
						const double dy = cells[index].row - centroidRow;
						const double dx = cells[index].col - centroidCol;
						const double distance = dy * dy + dx * dx;
						if (distance < nearestDistance)
						{
							nearestDistance = distance;
							nearest = index;
						}
						if (distance > farthestDistance)
						{
							farthestDistance = distance;
							farthest = index;
						}
					}
					plan.gateways.push_back(
						{.zoneA = key.first, .zoneB = key.second, .row = cells[nearest].row, .col = cells[nearest].col, .primary = true});
					if (cells.size() > 8 && rng.uniform() < 0.5)
					{
						plan.gateways.push_back(
							{.zoneA = key.first, .zoneB = key.second, .row = cells[farthest].row, .col = cells[farthest].col, .primary = false});
					}
				}
			}

			// ---------------- Stage E: settlements & POIs ----------------
			void placeEntities()
			{
				for (const PlanZone &zone : plan.zones)
				{
					Rng rng = rngFor("zone:" + std::to_string(zone.id) + "/settlements");
					std::vector<Cell> cells;
					for (int i = 0; i < size; ++i)
					{
						for (int j = 0; j < size; ++j)
						{
							if (plan.zone.at(i, j) == zone.id && plan.water.at(i, j) == 0 && isLand(i, j))
							{
								cells.push_back({i, j});
							}
						}
					}
					if (cells.empty())
					{
						plan.stats.warnings.push_back("zone " + std::to_string(zone.id) + ": no placeable land");
						continue;
					}
					const double zoneRadius = std::sqrt(cells.size() / 3.14159265358979323846);

					const auto placementOk = [&](int p_row, int p_col, PlanEntityKind p_kind, double p_block, double p_distRatio) {
						const bool isSettlement = p_kind == PlanEntityKind::Gym || p_kind == PlanEntityKind::City ||
												  p_kind == PlanEntityKind::PortCity;
						const double distMin = p_distRatio * zoneRadius;
						for (const PlanEntity &entity : plan.entities)
						{
							const double distance = std::hypot(p_row - entity.row, p_col - entity.col);
							if (distance < p_block)
							{
								return false;
							}
							const bool otherSettlement = entity.kind == PlanEntityKind::Gym || entity.kind == PlanEntityKind::City ||
														 entity.kind == PlanEntityKind::PortCity;
							if (isSettlement && otherSettlement && distance < distMin)
							{
								return false;
							}
						}
						return true;
					};

					enum class CoastRule { Any, Coastal, Inland };
					const auto sample = [&](PlanEntityKind p_kind, int p_count, double p_block, double p_distRatio, CoastRule p_rule) {
						int got = 0;
						std::vector<Cell> order = cells;
						rng.shuffle(order);
						int tries = 0;
						while (got < p_count && tries < 4000)
						{
							const Cell &candidate = order[tries % order.size()];
							++tries;
							const double dCoast = distOcean.at(candidate.row, candidate.col);
							if (p_rule == CoastRule::Coastal && dCoast > cfg.coastDistCells)
							{
								continue;
							}
							if (p_rule == CoastRule::Inland && dCoast <= cfg.coastDistCells)
							{
								continue;
							}
							if (!placementOk(candidate.row, candidate.col, p_kind, p_block, p_distRatio))
							{
								continue;
							}
							plan.entities.push_back({.kind = p_kind,
													 .row = candidate.row,
													 .col = candidate.col,
													 .zone = zone.id,
													 .continent = zoneContinent.contains(zone.id) ? zoneContinent[zone.id] : 1});
							++got;
						}
						return got;
					};

					// Gym: strictly inland; relax spacing, never the inland rule.
					int gyms = sample(PlanEntityKind::Gym, cfg.gymsPerZone, cfg.blockGym, cfg.distRatioGym, CoastRule::Inland);
					double relax = cfg.distRatioGym;
					while (gyms < cfg.gymsPerZone && relax > 0.05)
					{
						relax *= 0.5;
						gyms += sample(PlanEntityKind::Gym, cfg.gymsPerZone - gyms, cfg.blockGym * 0.6, relax, CoastRule::Inland);
					}
					if (gyms < cfg.gymsPerZone)
					{
						plan.stats.warnings.push_back(
							"zone " + std::to_string(zone.id) + ": gym quota " + std::to_string(gyms) + "/" +
							std::to_string(cfg.gymsPerZone) + " - no inland cell far from coast");
					}

					int cities = sample(PlanEntityKind::City, cfg.citiesPerZone, cfg.blockCity, cfg.distRatioCity, CoastRule::Inland);
					if (cities < cfg.citiesPerZone)
					{
						cities += sample(PlanEntityKind::City, cfg.citiesPerZone - cities, cfg.blockCity, cfg.distRatioCity, CoastRule::Any);
						if (cities < cfg.citiesPerZone)
						{
							plan.stats.warnings.push_back(
								"zone " + std::to_string(zone.id) + ": city quota " + std::to_string(cities) + "/" +
								std::to_string(cfg.citiesPerZone));
						}
					}

					sample(PlanEntityKind::RarePoi, cfg.rarePoiPerZone, cfg.blockRare, cfg.distRatioPoi, CoastRule::Any);
					sample(PlanEntityKind::UncommonPoi, cfg.uncommonPoiPerZone, cfg.blockUncommon, cfg.distRatioPoi, CoastRule::Any);
					sample(PlanEntityKind::NormalPoi, cfg.normalPoiPerZone, cfg.blockNormal, cfg.distRatioPoi, CoastRule::Any);
				}
				assignPorts();
			}

			void assignPorts()
			{
				std::map<int, std::vector<PlanEntity *>> byContinent;
				for (PlanEntity &entity : plan.entities)
				{
					if (entity.kind == PlanEntityKind::City)
					{
						byContinent[entity.continent].push_back(&entity);
					}
				}
				for (auto &[continent, cities] : byContinent)
				{
					std::sort(cities.begin(), cities.end(), [&](const PlanEntity *p_a, const PlanEntity *p_b) {
						return distOcean.at(p_a->row, p_a->col) < distOcean.at(p_b->row, p_b->col);
					});
					int made = 0;
					for (int index = 0; index < std::min<int>(cfg.portsPerContinent, static_cast<int>(cities.size())); ++index)
					{
						if (distOcean.at(cities[index]->row, cities[index]->col) <= cfg.coastDistCells * 2.0)
						{
							cities[index]->kind = PlanEntityKind::PortCity;
							++made;
						}
					}
					if (made < cfg.portsPerContinent)
					{
						plan.stats.warnings.push_back(
							"continent " + std::to_string(continent) + ": only " + std::to_string(made) + "/" +
							std::to_string(cfg.portsPerContinent) + " port cities");
					}
				}
			}

			// ---------------- Stage F: roads ----------------
			Rng roadRng{0};

			[[nodiscard]] double stepCost(int p_fromRow, int p_fromCol, int p_row, int p_col, const Cell &p_goal) const
			{
				if (!isLand(p_row, p_col) && !(Cell{p_row, p_col} == p_goal))
				{
					if (plan.water.at(p_row, p_col) == 0)
					{
						return -1.0; // open ocean: blocked for land roads
					}
				}
				double step = isLand(p_row, p_col) ? 1.0 : 1.0;
				if (plan.water.at(p_row, p_col) != 0)
				{
					step += 6.0; // bridge penalty
				}
				const int fromHeight = plan.height.at(p_fromRow, p_fromCol);
				const int toHeight = plan.height.at(p_row, p_col);
				if (fromHeight >= 0 && toHeight >= 0)
				{
					const int dh = std::abs(toHeight - fromHeight);
					if (dh >= 1)
					{
						step += 7.0 * dh; // stair penalty: roads contour around tall steps
					}
				}
				if (plan.road.at(p_row, p_col) != 0)
				{
					step *= 0.25; // strongly prefer reusing existing roads
				}
				return step;
			}

			// Diagonal-capable A* whose diagonal moves are materialized as an orthogonal
			// elbow chunk, so the emitted path is strictly orthogonal (port of _astar).
			[[nodiscard]] std::vector<Cell> findPath(const Cell &p_start, const Cell &p_goal)
			{
				const double SQRT2 = std::sqrt(2.0);
				const auto heuristic = [&](int p_row, int p_col) {
					const double dy = std::abs(p_row - p_goal.row);
					const double dx = std::abs(p_col - p_goal.col);
					return (dy + dx) + (SQRT2 - 2.0) * std::min(dy, dx);
				};
				const int cellCount = size * size;
				const auto indexOf = [&](int p_row, int p_col) { return p_row * size + p_col; };
				std::vector<double> best(cellCount, INF);
				std::vector<int> previous(cellCount, -1);
				std::vector<int> elbow(cellCount, -1);
				std::vector<std::uint8_t> reached(cellCount, 0);
				using Node = std::tuple<double, double, int>;
				std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
				const int startIndex = indexOf(p_start.row, p_start.col);
				const int goalIndex = indexOf(p_goal.row, p_goal.col);
				best[startIndex] = 0.0;
				open.push({heuristic(p_start.row, p_start.col), 0.0, startIndex});
				while (!open.empty())
				{
					const auto [f, g, current] = open.top();
					open.pop();
					if (current == goalIndex)
					{
						break;
					}
					if (g > best[current])
					{
						continue;
					}
					const int row = current / size;
					const int col = current % size;
					forNeighbors4(row, col, size, [&](int p_row, int p_col) {
						const double cost = stepCost(row, col, p_row, p_col, p_goal);
						if (cost < 0.0)
						{
							return;
						}
						const double candidate = g + cost;
						const int index = indexOf(p_row, p_col);
						if (candidate < best[index])
						{
							best[index] = candidate;
							previous[index] = current;
							elbow[index] = -1;
							reached[index] = 1;
							open.push({candidate + heuristic(p_row, p_col), candidate, index});
						}
					});
					for (const auto &[dr, dc] : {std::pair{-1, -1}, {-1, 1}, {1, -1}, {1, 1}})
					{
						const int nr = row + dr;
						const int nc = col + dc;
						if (nr < 0 || nc < 0 || nr >= size || nc >= size)
						{
							continue;
						}
						// A diagonal is legal only through a passable elbow; route the cheaper one.
						struct Option
						{
							double cost;
							int elbowIndex;
						};
						std::vector<Option> legal;
						for (const auto &[er, ec] : {std::pair{row + dr, col}, {row, col + dc}})
						{
							const double first = stepCost(row, col, er, ec, p_goal);
							if (first < 0.0)
							{
								continue;
							}
							const double second = stepCost(er, ec, nr, nc, p_goal);
							if (second < 0.0)
							{
								continue;
							}
							legal.push_back({(first + second) * (SQRT2 / 2.0), indexOf(er, ec)});
						}
						if (legal.empty())
						{
							continue;
						}
						std::sort(legal.begin(), legal.end(), [](const Option &p_a, const Option &p_b) { return p_a.cost < p_b.cost; });
						std::vector<int> tied;
						for (const Option &option : legal)
						{
							if (std::abs(option.cost - legal.front().cost) < 1e-9)
							{
								tied.push_back(option.elbowIndex);
							}
						}
						const int chosenElbow = tied.size() == 1 ? tied.front() : tied[roadRng.below(static_cast<int>(tied.size()))];
						const double candidate = g + legal.front().cost;
						const int index = indexOf(nr, nc);
						if (candidate < best[index])
						{
							best[index] = candidate;
							previous[index] = current;
							elbow[index] = chosenElbow;
							reached[index] = 1;
							open.push({candidate + heuristic(nr, nc), candidate, index});
						}
					}
				}
				if (reached[goalIndex] == 0 && goalIndex != startIndex)
				{
					return {};
				}
				std::vector<Cell> path{{p_goal.row, p_goal.col}};
				int cursor = goalIndex;
				while (cursor != startIndex)
				{
					const int prev = previous[cursor];
					if (prev < 0)
					{
						return {};
					}
					if (elbow[cursor] >= 0)
					{
						path.push_back({elbow[cursor] / size, elbow[cursor] % size});
					}
					path.push_back({prev / size, prev % size});
					cursor = prev;
				}
				std::reverse(path.begin(), path.end());
				return path;
			}

			bool connectRoad(const Cell &p_from, const Cell &p_to)
			{
				const std::vector<Cell> path = findPath(p_from, p_to);
				for (const Cell &cell : path)
				{
					plan.road.at(cell.row, cell.col) = 1;
				}
				return !path.empty();
			}

			[[nodiscard]] std::optional<Cell> nearestRoadCell(int p_row, int p_col, int p_maxRadius = 64) const
			{
				if (plan.road.at(p_row, p_col) != 0)
				{
					return Cell{p_row, p_col};
				}
				for (int radius = 1; radius <= p_maxRadius; ++radius)
				{
					std::optional<Cell> found;
					int foundDistance = std::numeric_limits<int>::max();
					const auto consider = [&](int p_r, int p_c) {
						if (p_r < 0 || p_c < 0 || p_r >= size || p_c >= size || plan.road.at(p_r, p_c) == 0)
						{
							return;
						}
						const int distance = std::abs(p_r - p_row) + std::abs(p_c - p_col);
						if (distance < foundDistance)
						{
							foundDistance = distance;
							found = Cell{p_r, p_c};
						}
					};
					for (int c = p_col - radius; c <= p_col + radius; ++c)
					{
						consider(p_row - radius, c);
						consider(p_row + radius, c);
					}
					for (int r = p_row - radius; r <= p_row + radius; ++r)
					{
						consider(r, p_col - radius);
						consider(r, p_col + radius);
					}
					if (found.has_value())
					{
						return found;
					}
				}
				return std::nullopt;
			}

			void buildRoads()
			{
				roadRng = rngFor("world/roads");
				std::map<int, std::vector<const PlanEntity *>> settlementsByZone;
				std::map<int, std::vector<const PlanEntity *>> poisByZone;
				for (const PlanEntity &entity : plan.entities)
				{
					if (entity.kind == PlanEntityKind::Gym || entity.kind == PlanEntityKind::City || entity.kind == PlanEntityKind::PortCity)
					{
						settlementsByZone[entity.zone].push_back(&entity);
					}
					else
					{
						poisByZone[entity.zone].push_back(&entity);
					}
				}

				std::map<int, Cell> hubs;
				for (const auto &[zone, settlements] : settlementsByZone)
				{
					const PlanEntity *hub = settlements.front();
					for (const PlanEntity *entity : settlements)
					{
						if (entity->kind == PlanEntityKind::Gym)
						{
							hub = entity;
							break;
						}
					}
					hubs[zone] = {hub->row, hub->col};
					plan.road.at(hub->row, hub->col) = 1;

					std::vector<const PlanEntity *> others;
					for (const PlanEntity *entity : settlements)
					{
						if (entity != hub)
						{
							others.push_back(entity);
						}
					}
					std::sort(others.begin(), others.end(), [&](const PlanEntity *p_a, const PlanEntity *p_b) {
						const int da = std::abs(p_a->row - hub->row) + std::abs(p_a->col - hub->col);
						const int db = std::abs(p_b->row - hub->row) + std::abs(p_b->col - hub->col);
						return da < db;
					});
					for (const PlanEntity *entity : others)
					{
						connectRoad(hubs[zone], {entity->row, entity->col});
					}

					// Wire a few POIs (rarity-blind) into the zone network, attaching to the
					// nearest existing road cell to avoid long spurs.
					const auto poiIterator = poisByZone.find(zone);
					if (poiIterator != poisByZone.end() && !poiIterator->second.empty())
					{
						Rng poiRng = rngFor("zone:" + std::to_string(zone) + "/poi_roads");
						std::vector<int> indices(poiIterator->second.size());
						for (std::size_t index = 0; index < indices.size(); ++index)
						{
							indices[index] = static_cast<int>(index);
						}
						poiRng.shuffle(indices);
						const int count = std::min<int>(cfg.poiRoadConnections, static_cast<int>(indices.size()));
						for (int pick = 0; pick < count; ++pick)
						{
							const PlanEntity *poi = poiIterator->second[indices[pick]];
							const std::optional<Cell> anchor = nearestRoadCell(poi->row, poi->col);
							if (anchor.has_value())
							{
								connectRoad(*anchor, {poi->row, poi->col});
							}
						}
					}
				}

				// Stitch zone networks through the primary gateways.
				for (const PlanGateway &gateway : plan.gateways)
				{
					if (!gateway.primary)
					{
						continue;
					}
					const auto hubA = hubs.find(gateway.zoneA);
					const auto hubB = hubs.find(gateway.zoneB);
					if (hubA != hubs.end() && hubB != hubs.end())
					{
						connectRoad(hubA->second, {gateway.row, gateway.col});
						connectRoad({gateway.row, gateway.col}, hubB->second);
					}
				}
			}

			void addBoatLinks()
			{
				int componentCount2 = 0;
				const PlanGrid<int> component = labelComponents(plan.road, componentCount2);
				plan.stats.roadComponents = componentCount2;
				if (componentCount2 <= 1)
				{
					return;
				}
				const auto componentOf = [&](const PlanEntity &p_entity) {
					if (plan.road.at(p_entity.row, p_entity.col) != 0)
					{
						return component.at(p_entity.row, p_entity.col);
					}
					for (int radius = 1; radius < 4; ++radius)
					{
						for (int dr = -radius; dr <= radius; ++dr)
						{
							for (int dc = -radius; dc <= radius; ++dc)
							{
								const int row = p_entity.row + dr;
								const int col = p_entity.col + dc;
								if (row >= 0 && col >= 0 && row < size && col < size && plan.road.at(row, col) != 0)
								{
									return component.at(row, col);
								}
							}
						}
					}
					return -1;
				};

				std::map<int, std::vector<PlanEntity *>> buckets;
				for (PlanEntity &entity : plan.entities)
				{
					if (entity.kind == PlanEntityKind::PortCity)
					{
						const int id = componentOf(entity);
						if (id > 0)
						{
							buckets[id].push_back(&entity);
						}
					}
				}
				// Every component needs a port to anchor a boat link; promote the most
				// coastal city of port-less components.
				for (int id = 1; id <= componentCount2; ++id)
				{
					if (buckets.contains(id))
					{
						continue;
					}
					PlanEntity *bestCity = nullptr;
					for (PlanEntity &entity : plan.entities)
					{
						if ((entity.kind == PlanEntityKind::City || entity.kind == PlanEntityKind::PortCity) && componentOf(entity) == id)
						{
							if (bestCity == nullptr ||
								distOcean.at(entity.row, entity.col) < distOcean.at(bestCity->row, bestCity->col))
							{
								bestCity = &entity;
							}
						}
					}
					if (bestCity != nullptr)
					{
						bestCity->kind = PlanEntityKind::PortCity;
						buckets[id].push_back(bestCity);
					}
				}
				if (buckets.size() < 2)
				{
					return;
				}
				const int baseComponent = buckets.begin()->first;
				for (const auto &[id, ports] : buckets)
				{
					if (id == baseComponent)
					{
						continue;
					}
					const PlanEntity *bestA = nullptr;
					const PlanEntity *bestB = nullptr;
					double bestDistance = INF;
					for (const PlanEntity *portA : buckets[baseComponent])
					{
						for (const PlanEntity *portB : ports)
						{
							const double distance = std::hypot(portA->row - portB->row, portA->col - portB->col);
							if (distance < bestDistance)
							{
								bestDistance = distance;
								bestA = portA;
								bestB = portB;
							}
						}
					}
					if (bestA != nullptr && bestB != nullptr)
					{
						plan.boatLinks.push_back({*bestA, *bestB});
					}
				}
			}

			// ---------------- Stage G: annotations -> prefab placements ----------------
			void markBridges()
			{
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						if (plan.road.at(i, j) != 0 && (plan.water.at(i, j) != 0 || !isLand(i, j)))
						{
							plan.bridge.at(i, j) = 1;
						}
					}
				}
			}

			// Prefab resolution: the zone's biome pool decides first, then the global
			// placements.json pool; entity kinds without any entry get no prefab. One
			// pool entry is picked at random per placement for diversity.
			Rng prefabPickRng{0};

			[[nodiscard]] const std::string &pickPrefab(const std::vector<std::string> &p_pool)
			{
				return p_pool.size() == 1 ? p_pool.front() : p_pool[prefabPickRng.below(static_cast<int>(p_pool.size()))];
			}

			// Stairway prefabs resolve by convention from the biome id: road climbs use
			// "<id>-road-stairway" (stairs), off-road/wild climbs "<id>-stairway" (slopes).
			// Cells outside any zone fall back to the shared "road-stairway".
			[[nodiscard]] std::string stairwayPrefabFor(int p_zone, bool p_onRoad) const
			{
				if (p_zone < 0)
				{
					return "road-stairway";
				}
				const std::string &biomeId = plan.biomes[plan.zones[p_zone].biomeIndex].id;
				return p_onRoad ? biomeId + "-road-stairway" : biomeId + "-stairway";
			}

			[[nodiscard]] const std::vector<std::string> *entityPrefabsFor(PlanEntityKind p_kind, int p_zone) const
			{
				if (p_zone >= 0)
				{
					const auto &overrides = plan.biomes[plan.zones[p_zone].biomeIndex].entityPrefabs;
					const auto found = overrides.find(p_kind);
					if (found != overrides.end())
					{
						return &found->second;
					}
				}
				const auto found = placementRules.entityPrefabs.find(p_kind);
				return found != placementRules.entityPrefabs.end() ? &found->second : nullptr;
			}

			// Lower cells of every stairway placed so far (road + wild), for spacing.
			std::vector<Cell> stairCells;

			// Chains one-level ramp prefabs inside the lower of the two cells so the
			// player can climb the strata difference; returns the number of segments.
			int emitStairChain(int p_row, int p_col, int p_dr, int p_dc, bool p_onRoad)
			{
				const int blocks = cfg.blocksPerCell;
				const int run = cfg.blocksPerLevel; // ramp run == ramp rise (cube prefab)
				const int offset = plan.worldOffset();
				const int strip = blocks / 2 - 1; // strip start (3-wide, centered)
				const int nr = p_row + p_dr;
				const int nc = p_col + p_dc;
				const int levelA = plan.height.at(p_row, p_col);
				const int levelB = plan.height.at(nr, nc);
				if (levelA == levelB)
				{
					return 0;
				}
				const bool firstLower = levelA < levelB;
				const int lowRow = firstLower ? p_row : nr;
				const int lowCol = firstLower ? p_col : nc;
				const int lowLevel = std::min(levelA, levelB);
				const int steps = std::min(std::abs(levelA - levelB), blocks / run);
				const int surface = plan.surfaceY(lowLevel);
				// One resolve per chain so every segment of a climb uses the same prefab.
				const std::string prefabId = stairwayPrefabFor(plan.zone.at(lowRow, lowCol), p_onRoad);

				for (int segment = 1; segment <= steps; ++segment)
				{
					PrefabPlacement placement;
					placement.prefabId = prefabId;
					placement.foundation = true;
					const int rise = surface + 1 + run * (segment - 1);
					const int distanceFromBoundary = run * (steps - segment); // 0 = against the boundary
					if (p_dc == 1)
					{
						// East/west neighbors: ramp runs along X.
						placement.orientation = firstLower ? spk::VoxelOrientation::PositiveX : spk::VoxelOrientation::NegativeX;
						const int boundary = offset + (p_col + 1) * blocks;
						const int minX = firstLower ? boundary - distanceFromBoundary - run : boundary + distanceFromBoundary;
						placement.anchor = {minX + run / 2, rise, offset + p_row * blocks + strip + 1};
					}
					else
					{
						// North/south neighbors: ramp runs along Z.
						placement.orientation = firstLower ? spk::VoxelOrientation::PositiveZ : spk::VoxelOrientation::NegativeZ;
						const int boundary = offset + (p_row + 1) * blocks;
						const int minZ = firstLower ? boundary - distanceFromBoundary - run : boundary + distanceFromBoundary;
						placement.anchor = {offset + p_col * blocks + strip + 1, rise, minZ + run / 2};
					}
					plan.placements.push_back(std::move(placement));
				}
				stairCells.push_back({lowRow, lowCol});
				return steps;
			}

			// Stairways: wherever the road steps between two strata levels, chain enough
			// one-level ramp prefabs inside the lower cell to climb the difference.
			void placeStairways()
			{
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						if (plan.road.at(i, j) == 0 || !isLand(i, j) || plan.bridge.at(i, j) != 0)
						{
							continue;
						}
						for (const auto &[dr, dc] : {std::pair{0, 1}, {1, 0}})
						{
							const int nr = i + dr;
							const int nc = j + dc;
							if (nr >= size || nc >= size || plan.road.at(nr, nc) == 0 || !isLand(nr, nc) ||
								plan.bridge.at(nr, nc) != 0)
							{
								continue;
							}
							plan.stats.stairPlacements += emitStairChain(i, j, dr, dc, true);
						}
					}
				}
			}

			// Wild staircases: off-road ramps across cliffs inside a zone, so exploration
			// is not funneled onto roads. Candidates avoid roads, water and entities and
			// keep their distance from every other stairway.
			void placeWildStairways()
			{
				if (cfg.wildStairsPerZone <= 0)
				{
					return;
				}
				const int maxSteps = cfg.blocksPerCell / cfg.blocksPerLevel;
				std::set<std::pair<int, int>> entityCells;
				for (const PlanEntity &entity : plan.entities)
				{
					entityCells.insert({entity.row, entity.col});
				}
				struct Candidate
				{
					int row;
					int col;
					int dr;
					int dc;
				};
				for (const PlanZone &zone : plan.zones)
				{
					Rng rng = rngFor("zone:" + std::to_string(zone.id) + "/wild_stairs");
					std::vector<Candidate> candidates;
					const auto usable = [&](int p_row, int p_col) {
						return plan.zone.contains(p_row, p_col) && plan.zone.at(p_row, p_col) == zone.id &&
							   isLand(p_row, p_col) && plan.road.at(p_row, p_col) == 0 &&
							   plan.water.at(p_row, p_col) == 0 && !entityCells.contains({p_row, p_col});
					};
					for (int i = 0; i < size; ++i)
					{
						for (int j = 0; j < size; ++j)
						{
							if (!usable(i, j))
							{
								continue;
							}
							for (const auto &[dr, dc] : {std::pair{0, 1}, {1, 0}})
							{
								if (!usable(i + dr, j + dc))
								{
									continue;
								}
								const int dh = std::abs(plan.height.at(i, j) - plan.height.at(i + dr, j + dc));
								if (dh >= 1 && dh <= maxSteps)
								{
									candidates.push_back({i, j, dr, dc});
								}
							}
						}
					}
					rng.shuffle(candidates);
					int placed = 0;
					for (const Candidate &candidate : candidates)
					{
						if (placed >= cfg.wildStairsPerZone)
						{
							break;
						}
						const bool tooClose = std::any_of(stairCells.begin(), stairCells.end(), [&](const Cell &p_cell) {
							return std::max(std::abs(p_cell.row - candidate.row), std::abs(p_cell.col - candidate.col)) <
								   cfg.wildStairSpacingCells;
						});
						if (tooClose)
						{
							continue;
						}
						const int segments = emitStairChain(candidate.row, candidate.col, candidate.dr, candidate.dc, false);
						if (segments > 0)
						{
							plan.wildStairs.push_back({stairCells.back().row, stairCells.back().col});
							plan.stats.wildStairPlacements += segments;
							++placed;
						}
					}
				}
			}

			void placeBuildings()
			{
				const int blocks = cfg.blocksPerCell;
				const int offset = plan.worldOffset();
				for (const PlanEntity &entity : plan.entities)
				{
					const std::vector<std::string> *pool = entityPrefabsFor(entity.kind, entity.zone);
					if (pool == nullptr || pool->empty())
					{
						continue; // this entity kind carries no prefab
					}
					const int level = plan.height.at(entity.row, entity.col);
					if (level < 0)
					{
						continue;
					}
					plan.placements.push_back({.prefabId = pickPrefab(*pool),
											   .anchor = {offset + entity.col * blocks + blocks / 2,
														  plan.surfaceY(level) + 1,
														  offset + entity.row * blocks + blocks / 2},
											   .orientation = spk::VoxelOrientation::PositiveZ,
											   .foundation = true});
				}
			}

			// ---------------- Stage H: validation ----------------
			void computeStats()
			{
				WorldPlanStats &stats = plan.stats;
				stats.continents = continentCount;
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						stats.landCells += isLand(i, j) ? 1 : 0;
						stats.roadCells += plan.road.at(i, j) != 0 ? 1 : 0;
						stats.riverCells += (plan.water.at(i, j) != 0 && plan.lake.at(i, j) == 0 && isLand(i, j)) ? 1 : 0;
					}
				}
				for (const PlanEntity &entity : plan.entities)
				{
					if (entity.kind == PlanEntityKind::Gym && distOcean.at(entity.row, entity.col) <= cfg.coastDistCells)
					{
						++stats.gymOnCoast;
					}
				}
				stats.roadDiagonalSteps = countDiagonalOnly(plan.road);
				Mask landWater(size, 0);
				for (int i = 0; i < size; ++i)
				{
					for (int j = 0; j < size; ++j)
					{
						landWater.at(i, j) = (plan.water.at(i, j) != 0 && isLand(i, j)) ? 1 : 0;
					}
				}
				stats.riverDiagonalSteps = countDiagonalOnly(landWater);
				for (const PlanGateway &gateway : plan.gateways)
				{
					(gateway.primary ? stats.primaryGateways : stats.secondaryGateways) += 1;
				}
				stats.boatLinks = static_cast<int>(plan.boatLinks.size());
				if (stats.roadComponents == 0)
				{
					int componentCount2 = 0;
					(void)labelComponents(plan.road, componentCount2);
					stats.roadComponents = componentCount2;
				}
			}

			[[nodiscard]] WorldPlan run() &&
			{
				buildWorldGraph();
				buildLandmass();
				assignZones();
				assignHeights();
				generateWater();
				resolveGateways();
				placeEntities();
				buildRoads();
				addBoatLinks();
				markBridges();
				prefabPickRng = rngFor("world/prefab_picks");
				placeStairways();
				placeWildStairways();
				placeBuildings();
				computeStats();
				return std::move(plan);
			}
		};
	}

	std::vector<PlanBiome> planBiomesFrom(const Registry<BiomeDefinition> &p_biomes)
	{
		std::vector<PlanBiome> result;
		for (const std::string &id : p_biomes.ids())
		{
			const BiomeDefinition &definition = p_biomes.get(id);
			if (!definition.worldgen.has_value())
			{
				continue; // interior biomes (caves, ...) never enter world generation
			}
			PlanBiome biome;
			biome.id = definition.id;
			biome.displayName = definition.displayName;
			biome.heightShift = definition.worldgen->heightShift;
			biome.peak = definition.worldgen->peak;
			biome.mapColor = definition.worldgen->mapColor;
			for (const auto &[slot, pool] : definition.worldgen->prefabs)
			{
				for (const auto &[key, kind] : planEntityKeyTable())
				{
					if (slot == key)
					{
						biome.entityPrefabs.emplace(kind, pool);
						break;
					}
				}
			}
			result.push_back(std::move(biome));
		}
		return result;
	}

	std::string WorldPlan::report() const
	{
		std::ostringstream out;
		const auto count = [&](PlanEntityKind p_kind) {
			int total = 0;
			for (const PlanEntity &entity : entities)
			{
				total += entity.kind == p_kind ? 1 : 0;
			}
			return total;
		};
		out << "================================================================\n";
		out << "WORLD REPORT  seed=" << config.masterSeed << "  size=" << config.size << "x" << config.size << " cells ("
			<< config.size * config.blocksPerCell << " blocks)\n";
		out << "================================================================\n";
		out << "zones................ " << zones.size() << "\n";
		for (const PlanZone &zone : zones)
		{
			out << "  Z" << zone.id << " -> " << biomes[zone.biomeIndex].displayName << " (progression "
				<< zone.progression << ")\n";
		}
		out << "continents........... " << stats.continents << "\n";
		out << "land cells........... " << stats.landCells << "\n";
		out << "river cells.......... " << stats.riverCells << "\n";
		out << "----------------------------------------------------------------\n";
		out << "gym cities........... " << count(PlanEntityKind::Gym) << "\n";
		out << "cities............... " << count(PlanEntityKind::City) << "\n";
		out << "port cities.......... " << count(PlanEntityKind::PortCity) << "\n";
		out << "rare POIs............ " << count(PlanEntityKind::RarePoi) << "\n";
		out << "uncommon POIs........ " << count(PlanEntityKind::UncommonPoi) << "\n";
		out << "normal POIs.......... " << count(PlanEntityKind::NormalPoi) << "\n";
		out << "----------------------------------------------------------------\n";
		out << "road cells........... " << stats.roadCells << "\n";
		out << "road components...... " << stats.roadComponents << "\n";
		out << "boat links........... " << stats.boatLinks << "\n";
		out << "logically connected.. " << ((stats.roadComponents - stats.boatLinks) <= 1 ? "true" : "FALSE") << "  (want true)\n";
		out << "gateways (primary)... " << stats.primaryGateways << "\n";
		out << "gateways (secondary). " << stats.secondaryGateways << "\n";
		out << "stair prefabs........ " << stats.stairPlacements << "\n";
		out << "wild stair prefabs... " << stats.wildStairPlacements << " (" << wildStairs.size() << " stairways)\n";
		out << "prefab placements.... " << placements.size() << "\n";
		out << "----------------------------------------------------------------\n";
		out << "gym on coast......... " << stats.gymOnCoast << "  (MUST be 0)\n";
		out << "road diagonal steps.. " << stats.roadDiagonalSteps << "  (MUST be 0)\n";
		out << "river diagonal steps. " << stats.riverDiagonalSteps << "  (MUST be 0)\n";
		if (!stats.warnings.empty())
		{
			out << "WARNINGS:\n";
			for (const std::string &warning : stats.warnings)
			{
				out << "  ! " << warning << "\n";
			}
		}
		out << "================================================================\n";
		return out.str();
	}

	WorldPlan generateWorldPlan(
		const WorldGenConfig &p_config,
		const std::vector<PlanBiome> &p_biomes,
		const PlanPlacementRules &p_placementRules)
	{
		return Generator(p_config, p_biomes, p_placementRules).run();
	}
}
