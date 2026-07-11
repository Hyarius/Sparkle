#include "world/generator/world_plan.hpp"

#include "core/registry.hpp"
#include "world/biome_definition.hpp"
#include "world/generator/placement_rules.hpp"
#include "world/interior_definition.hpp"
#include "world/prefab_definition.hpp"

#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iterator>
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

			explicit Rng(std::uint64_t p_seed) :
				engine(p_seed)
			{
			}

			[[nodiscard]] double uniform()
			{
				return std::uniform_real_distribution<double>(0.0, 1.0)(engine);
			}
			[[nodiscard]] int below(int p_count)
			{
				return std::uniform_int_distribution<int>(0, p_count - 1)(engine);
			}
			[[nodiscard]] int poisson(double p_mean)
			{
				return p_mean <= 0.0 ? 0 : std::poisson_distribution<int>(p_mean)(engine);
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
			const auto latticeAt = [&](int p_row, int p_col) {
				return lattice[static_cast<std::size_t>(p_row) * (n + 2) + p_col];
			};

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
					forNeighbors4(i, j, size, [&](int p_row, int p_col) {
						orthogonal |= p_mask.at(p_row, p_col) != 0;
					});
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
			const Registry<PrefabDefinition> &prefabs;
			const Registry<InteriorDefinition> &interiors;
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
				const PlanPlacementRules &p_placementRules,
				const Registry<PrefabDefinition> &p_prefabs,
				const Registry<InteriorDefinition> &p_interiors) :
				cfg(p_config),
				placementRules(p_placementRules),
				prefabs(p_prefabs),
				interiors(p_interiors),
				size(p_config.size)
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

			[[nodiscard]] bool isLand(int p_row, int p_col) const
			{
				return plan.land.at(p_row, p_col) != 0;
			}

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
					plan.zones.push_back({.id = index, .biomeIndex = biomeIndices[index % biomeIndices.size()], .progression = order[index]});
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

					enum class CoastRule
					{
						Any,
						Coastal,
						Inland
					};
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
							plan.entities.push_back({.kind = p_kind, .row = candidate.row, .col = candidate.col, .zone = zone.id, .continent = zoneContinent.contains(zone.id) ? zoneContinent[zone.id] : 1});
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

			// Roads stay one cell wide: a cell may not be laid if it would fill a 2x2 block
			// of road (four road units meeting in a square). Because riding on top of an
			// existing road is cheaper than running beside it, blocking the square just makes
			// parallel paths merge onto the same trunk instead of thickening it. Checks the
			// four 2x2 squares that contain (p_row, p_col) against the roads stamped so far.
			[[nodiscard]] bool wouldFormRoadSquare(int p_row, int p_col) const
			{
				const auto isRoad = [&](int p_r, int p_c) {
					return p_r >= 0 && p_c >= 0 && p_r < size && p_c < size && plan.road.at(p_r, p_c) != 0;
				};
				for (const auto &[dr, dc] : {std::pair{-1, -1}, {-1, 0}, {0, -1}, {0, 0}})
				{
					int roadCorners = 0;
					for (const auto &[cr, cc] : {std::pair{0, 0}, {0, 1}, {1, 0}, {1, 1}})
					{
						if (dr + cr == 0 && dc + cc == 0)
						{
							continue; // the cell we are about to lay
						}
						roadCorners += isRoad(p_row + dr + cr, p_col + dc + cc) ? 1 : 0;
					}
					if (roadCorners == 3)
					{
						return true; // the three other corners are road: laying here fills the square
					}
				}
				return false;
			}

			[[nodiscard]] double stepCost(int p_fromRow, int p_fromCol, int p_row, int p_col, const Cell &p_goal) const
			{
				if (!isLand(p_row, p_col) && !(Cell{p_row, p_col} == p_goal))
				{
					if (plan.water.at(p_row, p_col) == 0)
					{
						return -1.0; // open ocean: blocked for land roads
					}
				}
				if (plan.road.at(p_row, p_col) == 0 && !(Cell{p_row, p_col} == p_goal) && wouldFormRoadSquare(p_row, p_col))
				{
					return -1.0; // laying here would fill a 2x2 road square: route around it
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
					if (dh > cfg.maxComposedStairLevels)
					{
						return -1.0; // no composed staircase can bridge this edge
					}
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
				const auto indexOf = [&](int p_row, int p_col) {
					return p_row * size + p_col;
				};
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
						std::sort(legal.begin(), legal.end(), [](const Option &p_a, const Option &p_b) {
							return p_a.cost < p_b.cost;
						});
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
				removeRoadSquares();
			}

			// Guarantees roads stay one cell wide. The A* guard keeps parallel paths from
			// thickening, but the diagonal-elbow reconstruction and forced junctions can still
			// leave a stray 2x2 block; this erases the redundant corner of each one. A cell is
			// removed only when both of its neighbours outside the square are non-road, so its
			// sole links are the two square siblings, which stay connected through the diagonal
			// sibling (an L) -- nothing external can be cut off. True 4-way crossings, where
			// every corner has an outside road neighbour, are left untouched.
			void removeRoadSquares()
			{
				const auto isRoad = [&](int p_r, int p_c) {
					return p_r >= 0 && p_c >= 0 && p_r < size && p_c < size && plan.road.at(p_r, p_c) != 0;
				};
				bool changed = true;
				while (changed)
				{
					changed = false;
					for (int i = 0; i + 1 < size && !changed; ++i)
					{
						for (int j = 0; j + 1 < size && !changed; ++j)
						{
							if (!(isRoad(i, j) && isRoad(i, j + 1) && isRoad(i + 1, j) && isRoad(i + 1, j + 1)))
							{
								continue;
							}
							// Each square corner paired with its two orthogonal neighbours that
							// lie outside the 2x2 block.
							const std::array<std::array<int, 6>, 4> corners = {{
								{i, j, i - 1, j, i, j - 1},
								{i, j + 1, i - 1, j + 1, i, j + 2},
								{i + 1, j, i + 2, j, i + 1, j - 1},
								{i + 1, j + 1, i + 2, j + 1, i + 1, j + 2},
							}};
							for (const auto &corner : corners)
							{
								if (!isRoad(corner[2], corner[3]) && !isRoad(corner[4], corner[5]))
								{
									plan.road.at(corner[0], corner[1]) = 0;
									changed = true;
									break;
								}
							}
						}
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

			// Stair prefabs are synthesized from each biome's palette voxels (see
			// synthesizeClimbPrefabs) and carried on PlanBiome as id pools: road climbs draw
			// from the biome's stair pool, wild climbs from its slope pool. Flight pools hold
			// several pre-mixed variants; pickStairLength draws one per segment for variety. A
			// biome without stair/slope voxels, or a cell outside any zone, falls back to the
			// shared "stair-length"/"stair-platform" pair.
			[[nodiscard]] std::vector<std::string> stairLengthPoolFor(int p_zone, bool p_onRoad) const
			{
				if (p_zone >= 0)
				{
					const PlanBiome &biome = plan.biomes[plan.zones[p_zone].biomeIndex];
					const std::vector<std::string> &pool = p_onRoad ? biome.roadStairLengths : biome.wildSlopeLengths;
					if (!pool.empty())
					{
						return pool;
					}
				}
				return {"stair-length"};
			}

			std::string pickStairLength(int p_zone, bool p_onRoad)
			{
				return pickPrefab(stairLengthPoolFor(p_zone, p_onRoad));
			}

			[[nodiscard]] std::string stairPlatformPrefabFor(int p_zone, bool p_onRoad) const
			{
				if (p_zone >= 0)
				{
					const PlanBiome &biome = plan.biomes[plan.zones[p_zone].biomeIndex];
					const std::string &id = p_onRoad ? biome.roadStairPlatform : biome.wildSlopePlatform;
					if (!id.empty())
					{
						return id;
					}
				}
				return "stair-platform";
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

			// ---------------- Claimed zones ----------------
			// Every structural placement (stairways first — they have priority — then
			// buildings, then interior rooms) claims a world-space box: its authored
			// "clearance" zone when the prefab declares one, its content bounds otherwise.
			// Later placements only commit if their own claimed zone is entirely free, so
			// stairs can no longer merge into POIs and vice versa.
			struct Claim
			{
				spk::Vector3Int min{};
				spk::Vector3Int max{};
			};
			std::vector<Claim> hardClaims;

			struct ResolvedBox
			{
				spk::Vector3Int worldMin{};	   // min corner of the rotated content box
				spk::Vector3Int extents{};	   // its size per axis
				spk::Vector3Int destination{}; // world cell the prefab's pivot lands on
			};

			// Mirrors PlanChunkProvider's placement resolution so the plan reasons about
			// the exact voxels the realization will stamp.
			[[nodiscard]] std::optional<ResolvedBox> resolveBox(const PrefabPlacement &p_placement) const
			{
				const PrefabDefinition *definition = prefabs.tryGet(p_placement.prefabId);
				if (definition == nullptr)
				{
					return std::nullopt;
				}
				const auto [rotatedMin, rotatedMax] = definition->prefab.rotatedBounds(p_placement.orientation);
				const spk::Vector3Int extents = rotatedMax - rotatedMin + spk::Vector3Int{1, 1, 1};
				const spk::Vector3Int worldMin = p_placement.anchorToPivot
													 ? p_placement.anchor + rotatedMin
													 : spk::Vector3Int{
														   p_placement.anchor.x - extents.x / 2,
														   p_placement.anchor.y + rotatedMin.y,
														   p_placement.anchor.z - extents.z / 2};
				return ResolvedBox{
					.worldMin = worldMin,
					.extents = extents,
					.destination = p_placement.anchorToPivot ? p_placement.anchor : worldMin - rotatedMin};
			}

			[[nodiscard]] std::optional<Claim> claimBoxFor(const PrefabPlacement &p_placement) const
			{
				const PrefabDefinition *definition = prefabs.tryGet(p_placement.prefabId);
				const std::optional<ResolvedBox> resolved = resolveBox(p_placement);
				if (definition == nullptr || !resolved.has_value())
				{
					return std::nullopt;
				}
				const spk::Vector3Int pivot = definition->prefab.pivot();
				const spk::Vector3Int localMin =
					definition->clearance.has_value() ? definition->clearance->min : definition->prefab.minBounds();
				const spk::Vector3Int localMax =
					definition->clearance.has_value() ? definition->clearance->max : definition->prefab.maxBounds();
				// Same quarter-turn convention as spk::Prefab (rotations around +Y through
				// the pivot), so claimed zones rotate exactly like the content.
				const int turns = spk::quarterTurnsOf(p_placement.orientation);
				const spk::Vector3Int a = spk::rotateQuarterTurns(localMin - pivot, turns);
				const spk::Vector3Int b = spk::rotateQuarterTurns(localMax - pivot, turns);
				return Claim{
					.min = resolved->destination + spk::Vector3Int{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
					.max = resolved->destination + spk::Vector3Int{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}};
			}

			[[nodiscard]] static bool claimsOverlap(const Claim &p_first, const Claim &p_second)
			{
				return p_first.min.x <= p_second.max.x && p_first.max.x >= p_second.min.x &&
					   p_first.min.y <= p_second.max.y && p_first.max.y >= p_second.min.y &&
					   p_first.min.z <= p_second.max.z && p_first.max.z >= p_second.min.z;
			}

			[[nodiscard]] bool zoneIsFree(const Claim &p_claim) const
			{
				return std::ranges::none_of(hardClaims, [&](const Claim &p_placed) {
					return claimsOverlap(p_claim, p_placed);
				});
			}

			// Lower cells of every stairway placed so far (road + wild), for spacing.
			std::vector<Cell> stairCells;
			struct StairFootprint
			{
				int minX = 0;
				int maxX = 0;
				int minZ = 0;
				int maxZ = 0;
			};
			std::vector<StairFootprint> stairFootprints;

			[[nodiscard]] std::optional<StairFootprint> stairFootprintOf(const PrefabPlacement &p_placement) const
			{
				const PrefabDefinition *definition = prefabs.tryGet(p_placement.prefabId);
				if (definition == nullptr)
				{
					return std::nullopt;
				}
				const auto [rotatedMin, rotatedMax] = definition->prefab.rotatedBounds(p_placement.orientation);
				const spk::Vector3Int extents = rotatedMax - rotatedMin + spk::Vector3Int{1, 1, 1};
				const spk::Vector3Int worldMin = p_placement.anchorToPivot
													 ? p_placement.anchor + rotatedMin
													 : spk::Vector3Int{
														   p_placement.anchor.x - extents.x / 2,
														   p_placement.anchor.y + rotatedMin.y,
														   p_placement.anchor.z - extents.z / 2};
				return StairFootprint{
					.minX = worldMin.x,
					.maxX = worldMin.x + extents.x - 1,
					.minZ = worldMin.z,
					.maxZ = worldMin.z + extents.z - 1};
			}

			[[nodiscard]] static bool footprintsOverlap(
				const StairFootprint &p_first,
				const StairFootprint &p_second)
			{
				return p_first.minX <= p_second.maxX && p_first.maxX >= p_second.minX &&
					   p_first.minZ <= p_second.maxZ && p_first.maxZ >= p_second.minZ;
			}

			[[nodiscard]] bool planCellHasEntity(int p_row, int p_col) const
			{
				return std::ranges::any_of(plan.entities, [&](const PlanEntity &p_entity) {
					return p_entity.row == p_row && p_entity.col == p_col;
				});
			}

			// Road interaction of a stair footprint: straight ramps crossing on the road
			// network must stay on road cells, composed climbs may also use clear
			// terrain beside the road, and wild stairways must not touch roads at all.
			enum class RoadRule
			{
				Require,
				Allow,
				Forbid
			};

			[[nodiscard]] bool stairFootprintFits(
				const StairFootprint &p_footprint,
				int p_lowLevel,
				int p_lowZone,
				RoadRule p_roadRule,
				const std::vector<StairFootprint> &p_group) const
			{
				for (const StairFootprint &placed : stairFootprints)
				{
					if (footprintsOverlap(p_footprint, placed))
					{
						return false;
					}
				}
				for (const StairFootprint &placed : p_group)
				{
					if (footprintsOverlap(p_footprint, placed))
					{
						return false;
					}
				}

				for (int worldZ = p_footprint.minZ; worldZ <= p_footprint.maxZ; ++worldZ)
				{
					for (int worldX = p_footprint.minX; worldX <= p_footprint.maxX; ++worldX)
					{
						const int row = plan.cellIndexFromWorld(worldZ);
						const int col = plan.cellIndexFromWorld(worldX);
						if (!plan.land.contains(row, col) || !isLand(row, col) ||
							plan.height.at(row, col) != p_lowLevel || plan.water.at(row, col) != 0 ||
							plan.bridge.at(row, col) != 0 || planCellHasEntity(row, col))
						{
							return false;
						}
						switch (p_roadRule)
						{
						case RoadRule::Require:
							if (plan.road.at(row, col) == 0)
							{
								return false;
							}
							break;
						case RoadRule::Allow:
							if (plan.zone.at(row, col) != p_lowZone)
							{
								return false;
							}
							break;
						case RoadRule::Forbid:
							if (plan.road.at(row, col) != 0 || plan.zone.at(row, col) != p_lowZone)
							{
								return false;
							}
							break;
						}
					}
				}
				return true;
			}

			bool commitStairGroup(
				std::vector<PrefabPlacement> p_placements,
				int p_lowLevel,
				int p_lowZone,
				RoadRule p_roadRule,
				const std::vector<StairFootprint> &p_extraFootprints = {},
				const std::vector<StairFootprint> &p_reservedFootprints = {},
				const std::vector<Claim> &p_extraClaims = {})
			{
				std::vector<StairFootprint> footprints;
				footprints.reserve(p_placements.size() + p_extraFootprints.size() + p_reservedFootprints.size());
				for (const PrefabPlacement &placement : p_placements)
				{
					const std::optional<StairFootprint> footprint = stairFootprintOf(placement);
					if (!footprint.has_value() ||
						!stairFootprintFits(*footprint, p_lowLevel, p_lowZone, p_roadRule, footprints))
					{
						return false;
					}
					footprints.push_back(*footprint);
				}
				// Extra check-only rectangles (the approach lane beside a composed climb):
				// validated and reserved exactly like stamped footprints, never realized.
				for (const StairFootprint &extra : p_extraFootprints)
				{
					if (!stairFootprintFits(extra, p_lowLevel, p_lowZone, p_roadRule, footprints))
					{
						return false;
					}
					footprints.push_back(extra);
				}
				// Reserved rectangles (the exit cells on the high plateau): only tested
				// against other stairways, then recorded, so two flights can never meet
				// face to face across a boundary. Their terrain is validated by the caller.
				for (const StairFootprint &reserved : p_reservedFootprints)
				{
					const bool blocked =
						std::ranges::any_of(
							stairFootprints,
							[&](const StairFootprint &p_placed) { return footprintsOverlap(reserved, p_placed); }) ||
						std::ranges::any_of(footprints, [&](const StairFootprint &p_placed) {
							return footprintsOverlap(reserved, p_placed);
						});
					if (blocked)
					{
						return false;
					}
					footprints.push_back(reserved);
				}
				// Stairways are placed first and have priority: they claim their zones
				// unconditionally, and everything placed later must keep clear of them.
				for (const PrefabPlacement &placement : p_placements)
				{
					if (const std::optional<Claim> claim = claimBoxFor(placement); claim.has_value())
					{
						hardClaims.push_back(*claim);
					}
				}
				hardClaims.insert(hardClaims.end(), p_extraClaims.begin(), p_extraClaims.end());
				plan.placements.insert(
					plan.placements.end(),
					std::make_move_iterator(p_placements.begin()),
					std::make_move_iterator(p_placements.end()));
				stairFootprints.insert(stairFootprints.end(), footprints.begin(), footprints.end());
				// The preview map repaints these rectangles in road color: where a climb
				// detours the walked path, the drawn road follows the actual structure.
				for (const StairFootprint &footprint : footprints)
				{
					plan.stairRects.push_back(
						{.minX = footprint.minX, .maxX = footprint.maxX, .minZ = footprint.minZ, .maxZ = footprint.maxZ});
				}
				for (const StairFootprint &footprint : footprints)
				{
					const int minRow = plan.cellIndexFromWorld(footprint.minZ);
					const int maxRow = plan.cellIndexFromWorld(footprint.maxZ);
					const int minCol = plan.cellIndexFromWorld(footprint.minX);
					const int maxCol = plan.cellIndexFromWorld(footprint.maxX);
					for (int row = minRow; row <= maxRow; ++row)
					{
						for (int col = minCol; col <= maxCol; ++col)
						{
							if (std::ranges::find(stairCells, Cell{row, col}) == stairCells.end())
							{
								stairCells.push_back({row, col});
							}
						}
					}
				}
				return true;
			}

			// Places an adaptive staircase across a cliff. One-level climbs keep the
			// straight centered ramp crossing the boundary. Taller climbs compose a
			// 3x3 stair-platform, one stair-length per height level, and a second
			// 3x3 stair-platform, the whole run hugging the cliff wall on the low
			// side so the top platform's surface meets the high plateau flush across
			// the boundary. Returns the number of emitted prefab placements.
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
				const int highLevel = std::max(levelA, levelB);
				const int lowZone = plan.zone.at(lowRow, lowCol);
				const int steps = highLevel - lowLevel;
				const int surface = plan.surfaceY(lowLevel);
				const int highSurface = plan.surfaceY(highLevel);

				const int maximumLevels = p_onRoad ? cfg.maxComposedStairLevels : cfg.maxWildStairLevels;
				if (steps > maximumLevels)
				{
					++plan.stats.rejectedStairways;
					return 0;
				}

				if (steps >= 2)
				{
					// Composed staircase. Local frame: "across" is the horizontal axis
					// perpendicular to the cliff, "along" the axis running beside it. The
					// three across-columns nearest the wall carry the platforms and the
					// flight; the fourth is a checked, unstamped walkway lane connecting
					// the road dead-end below the top platform to the bottom platform.
					const std::string platformId = stairPlatformPrefabFor(lowZone, p_onRoad);
					const int boundary =
						p_dc == 1 ? offset + (p_col + 1) * blocks : offset + (p_row + 1) * blocks;
					const int wallSide = firstLower ? -1 : 1; // away from the wall, low side
					const int wallColumn = firstLower ? boundary - 1 : boundary;
					const int acrossCenter = wallColumn + wallSide;
					// The three columns beyond the structure form the paved approach band:
					// the road-width path from the crossing dead-end to the bottom platform.
					const int bandNearColumn = wallColumn + 3 * wallSide;
					const int bandFarColumn = wallColumn + 5 * wallSide;
					const int alongCenterBase =
						(p_dc == 1 ? offset + p_row * blocks : offset + p_col * blocks) + strip + 1;
					// The stair-length prefab climbs its local +Z; pick the rotation whose
					// climb runs along the cliff, against the flight's descent direction.
					const spk::VoxelOrientation ascendPositive =
						p_dc == 1 ? spk::VoxelOrientation::PositiveZ : spk::VoxelOrientation::PositiveX;
					const spk::VoxelOrientation ascendNegative =
						p_dc == 1 ? spk::VoxelOrientation::NegativeZ : spk::VoxelOrientation::NegativeX;
					const auto anchorAt = [&](int p_across, int p_y, int p_along) {
						return p_dc == 1 ? spk::Vector3Int{p_across, p_y, p_along}
										 : spk::Vector3Int{p_along, p_y, p_across};
					};

					constexpr std::array tangents = {1, -1};
					constexpr std::array crossOffsets = {0, -1, 1};
					for (const int tangent : tangents)
					{
						for (const int crossOffset : crossOffsets)
						{
							// The top platform pads the 3-wide road strip of the crossing, so
							// stepping over the boundary lands on it flush; the flight then
							// descends beside the cliff toward the bottom platform.
							const int alongCenter = alongCenterBase + crossOffset;
							std::vector<PrefabPlacement> placements;
							placements.reserve(static_cast<std::size_t>(steps) + 2);

							PrefabPlacement top;
							top.prefabId = platformId;
							top.foundation = true;
							top.anchor = anchorAt(acrossCenter, highSurface + 1, alongCenter);
							placements.push_back(std::move(top));

							for (int segment = 1; segment <= steps; ++segment)
							{
								PrefabPlacement piece;
								piece.prefabId = pickStairLength(lowZone, p_onRoad);
								piece.orientation = tangent == 1 ? ascendNegative : ascendPositive;
								piece.foundation = true;
								piece.anchor = anchorAt(
									acrossCenter,
									surface + 1 + run * (steps - segment),
									alongCenter + tangent * run * segment);
								placements.push_back(std::move(piece));
							}

							PrefabPlacement bottom;
							bottom.prefabId = platformId;
							bottom.foundation = true;
							bottom.anchor =
								anchorAt(acrossCenter, surface + 1, alongCenter + tangent * run * (steps + 1));
							placements.push_back(std::move(bottom));

							const int nearEdge = alongCenter - tangent;
							const int farEdge = alongCenter + tangent * (run * (steps + 1) + 1);
							const int alongMin = std::min(nearEdge, farEdge);
							const int alongMax = std::max(nearEdge, farEdge);
							const int bandMin = std::min(bandNearColumn, bandFarColumn);
							const int bandMax = std::max(bandNearColumn, bandFarColumn);
							const StairFootprint band =
								p_dc == 1 ? StairFootprint{bandMin, bandMax, alongMin, alongMax}
										  : StairFootprint{alongMin, alongMax, bandMin, bandMax};

							// The three high-plateau cells the top platform opens onto must be
							// dry, level land, and are reserved so no other flight or building
							// ever blocks the exit face to face.
							const int exitColumn = wallColumn - wallSide;
							bool exitClear = true;
							for (int along = alongCenter - 1; along <= alongCenter + 1 && exitClear; ++along)
							{
								const int row = plan.cellIndexFromWorld(p_dc == 1 ? along : exitColumn);
								const int col = plan.cellIndexFromWorld(p_dc == 1 ? exitColumn : along);
								exitClear = plan.land.contains(row, col) && isLand(row, col) &&
											plan.height.at(row, col) == highLevel &&
											plan.water.at(row, col) == 0 && plan.bridge.at(row, col) == 0;
							}
							if (!exitClear)
							{
								continue;
							}
							const StairFootprint exit =
								p_dc == 1
									? StairFootprint{exitColumn, exitColumn, alongCenter - 1, alongCenter + 1}
									: StairFootprint{alongCenter - 1, alongCenter + 1, exitColumn, exitColumn};
							const Claim exitClaim =
								p_dc == 1
									? Claim{{exitColumn, highSurface + 1, alongCenter - 1},
											{exitColumn, highSurface + 4, alongCenter + 1}}
									: Claim{{alongCenter - 1, highSurface + 1, exitColumn},
											{alongCenter + 1, highSurface + 4, exitColumn}};
							// One claim over the whole structure, approach band included, from
							// the low ground to above the top platform: keeps scenery and
							// buildings out from under the flight and off the approach.
							const int acrossMin = std::min(wallColumn, bandFarColumn);
							const int acrossMax = std::max(wallColumn, bandFarColumn);
							const Claim stripClaim =
								p_dc == 1
									? Claim{{acrossMin, surface, alongMin}, {acrossMax, highSurface + 4, alongMax}}
									: Claim{{alongMin, surface, acrossMin}, {alongMax, highSurface + 4, acrossMax}};

							if (commitStairGroup(
									std::move(placements),
									lowLevel,
									lowZone,
									p_onRoad ? RoadRule::Allow : RoadRule::Forbid,
									{band},
									{exit},
									{stripClaim, exitClaim}))
							{
								// Only road climbs pave their approach; wild ones keep the
								// validated band as untouched natural ground.
								if (p_onRoad)
								{
									plan.pavedRects.push_back(
										{.minX = band.minX, .maxX = band.maxX, .minZ = band.minZ, .maxZ = band.maxZ});
								}
								++plan.stats.composedStairPlacements;
								stairCells.push_back({lowRow, lowCol});
								return steps + 2;
							}
						}
					}
					// A two-level transition still has a valid centered straight layout;
					// keep that fallback when the composed footprint is obstructed.
					if (p_onRoad && steps > cfg.maxRoadStairLevels)
					{
						++plan.stats.rejectedStairways;
						return 0;
					}
				}
				std::vector<PrefabPlacement> placements;
				placements.reserve(static_cast<std::size_t>(steps));

				for (int segment = 1; segment <= steps; ++segment)
				{
					PrefabPlacement placement;
					// Each segment draws its own flight variant so a climb reads as a mix.
					placement.prefabId = pickStairLength(lowZone, p_onRoad);
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
					placements.push_back(std::move(placement));
				}
				if (!commitStairGroup(
						std::move(placements),
						lowLevel,
						lowZone,
						p_onRoad ? RoadRule::Require : RoadRule::Forbid))
				{
					++plan.stats.rejectedStairways;
					return 0;
				}
				stairCells.push_back({lowRow, lowCol});
				return steps;
			}

			// Stairways: wherever the road steps between two strata levels, place an
			// adaptive staircase sized to the complete height difference.
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
				// Composed staircases climb any wild cliff up to the configured cap; the
				// candidate scan only proposes edges the composer can actually serve.
				const int maxSteps = std::min(cfg.maxWildStairLevels, cfg.maxHeightLevel);
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

			// ---------------- Stage G bis: interiors ----------------
			int interiorSlotsUsed = 0;

			// Room prefabs connect through anchors named "connector:<axis>", placed on
			// the wall block they can open through, one cell above the room floor.
			[[nodiscard]] static std::optional<spk::Vector3Int> connectorDirection(const std::string &p_name)
			{
				if (!p_name.starts_with("connector:"))
				{
					return std::nullopt;
				}
				const std::string_view axis = std::string_view(p_name).substr(10);
				if (axis == "+x")
				{
					return spk::Vector3Int{1, 0, 0};
				}
				if (axis == "-x")
				{
					return spk::Vector3Int{-1, 0, 0};
				}
				if (axis == "+z")
				{
					return spk::Vector3Int{0, 0, 1};
				}
				if (axis == "-z")
				{
					return spk::Vector3Int{0, 0, -1};
				}
				return std::nullopt;
			}

			[[nodiscard]] static const InteriorRoomOption &pickWeightedRoom(
				Rng &p_rng,
				const std::vector<InteriorRoomOption> &p_options)
			{
				double total = 0.0;
				for (const InteriorRoomOption &option : p_options)
				{
					total += option.weight;
				}
				double roll = p_rng.uniform() * total;
				for (const InteriorRoomOption &option : p_options)
				{
					roll -= option.weight;
					if (roll <= 0.0)
					{
						return option;
					}
				}
				return p_options.back();
			}

			// Composes the interior linked by a just-placed building: grows a seeded set
			// of rooms from the entry room inside a reserved void slot east of the world,
			// carves the doorways between connected rooms, and pairs the building's door
			// with the entry/exit pads through two one-way portals.
			void composeInterior(const PrefabPlacement &p_buildingPlacement)
			{
				const PrefabDefinition *building = prefabs.tryGet(p_buildingPlacement.prefabId);
				if (building == nullptr || building->interiorId.empty())
				{
					return;
				}
				const InteriorDefinition *interior = interiors.tryGet(building->interiorId);
				const PrefabAnchor *door = building->tryAnchor("door");
				const PrefabDefinition *entryRoom =
					interior == nullptr ? nullptr : prefabs.tryGet(interior->entryPrefabId);
				const PrefabAnchor *entryPad = entryRoom == nullptr ? nullptr : entryRoom->tryAnchor("entry");
				const PrefabAnchor *exitPad = entryRoom == nullptr ? nullptr : entryRoom->tryAnchor("exit");
				const std::optional<ResolvedBox> buildingBox = resolveBox(p_buildingPlacement);
				if (interior == nullptr || door == nullptr || entryPad == nullptr || exitPad == nullptr ||
					!buildingBox.has_value())
				{
					plan.stats.warnings.push_back(
						"interior '" + building->interiorId + "' of prefab '" + building->id + "' could not be composed");
					return;
				}

				// The door cell is the floor block inside the doorway (anchored at local
				// y = -1); the return portal drops the player one cell outside it, on the
				// same flattened plateau. Buildings stamp with the identity orientation,
				// so anchors resolve untransformed.
				const spk::Vector3Int buildingPivot = building->prefab.pivot();
				const spk::Vector3Int doorWorld = buildingBox->destination + door->at - buildingPivot;
				const spk::Vector3Int minBoundsB = building->prefab.minBounds();
				const spk::Vector3Int maxBoundsB = building->prefab.maxBounds();
				const double doorOffsetX = door->at.x - (minBoundsB.x + maxBoundsB.x) / 2.0;
				const double doorOffsetZ = door->at.z - (minBoundsB.z + maxBoundsB.z) / 2.0;
				const spk::Vector3Int outward = std::abs(doorOffsetX) > std::abs(doorOffsetZ)
													? spk::Vector3Int{doorOffsetX >= 0 ? 1 : -1, 0, 0}
													: spk::Vector3Int{0, 0, doorOffsetZ >= 0 ? 1 : -1};

				// One square void slot per interior, along the +Z edge of the band. Rooms
				// share the level-0 ground height so walk heights match the overworld.
				const int slotIndex = interiorSlotsUsed++;
				const int slotSide = cfg.interiorSlotBlocks;
				const int groundY = plan.surfaceY(0) + 1;
				const Claim slotBounds{
					.min = {plan.interiorRegionMinX(), 0, plan.worldOffset() + slotIndex * slotSide},
					.max = {plan.interiorRegionMinX() + slotSide - 1, 63, plan.worldOffset() + (slotIndex + 1) * slotSide - 1}};
				Rng rng = rngFor("interior:" + std::to_string(slotIndex));

				struct OpenConnector
				{
					spk::Vector3Int at{}; // world wall cell the connector opens through
					spk::Vector3Int direction{};
				};
				std::vector<Claim> roomBoxes;
				std::vector<OpenConnector> open;

				const auto placeRoom = [&](const PrefabDefinition &p_room, const spk::Vector3Int &p_destination) {
					plan.placements.push_back(
						{.prefabId = p_room.id,
						 .anchor = p_destination,
						 .orientation = spk::VoxelOrientation::PositiveZ,
						 .foundation = false,
						 .anchorToPivot = true});
					const spk::Vector3Int pivot = p_room.prefab.pivot();
					roomBoxes.push_back(
						{.min = p_destination + p_room.prefab.minBounds() - pivot,
						 .max = p_destination + p_room.prefab.maxBounds() - pivot});
					hardClaims.push_back(roomBoxes.back());
					for (const PrefabAnchor &anchor : p_room.anchors)
					{
						if (const std::optional<spk::Vector3Int> direction = connectorDirection(anchor.name))
						{
							open.push_back({.at = p_destination + anchor.at - pivot, .direction = *direction});
						}
					}
					++plan.stats.interiorRoomPlacements;
				};

				const spk::Vector3Int entryDestination{
					slotBounds.min.x + slotSide / 2, groundY, slotBounds.min.z + slotSide / 2};
				placeRoom(*entryRoom, entryDestination);

				const int extraRange = interior->maxRooms - interior->minRooms;
				const int extraTarget = interior->minRooms + (extraRange > 0 ? rng.below(extraRange + 1) : 0);
				int extraPlaced = 0;
				for (int attempt = 0; attempt < 64 && extraPlaced < extraTarget && !open.empty(); ++attempt)
				{
					const OpenConnector connector = open[rng.below(static_cast<int>(open.size()))];
					const InteriorRoomOption &option = pickWeightedRoom(rng, interior->rooms);
					const PrefabDefinition *candidate = prefabs.tryGet(option.prefabId);
					if (candidate == nullptr)
					{
						continue;
					}
					const spk::Vector3Int inwardDirection{
						-connector.direction.x, -connector.direction.y, -connector.direction.z};
					std::vector<const PrefabAnchor *> mates;
					for (const PrefabAnchor &anchor : candidate->anchors)
					{
						const std::optional<spk::Vector3Int> direction = connectorDirection(anchor.name);
						if (direction.has_value() && *direction == inwardDirection)
						{
							mates.push_back(&anchor);
						}
					}
					if (mates.empty())
					{
						continue;
					}
					const PrefabAnchor *mate = mates[rng.below(static_cast<int>(mates.size()))];
					const spk::Vector3Int mateWall = connector.at + connector.direction;
					const spk::Vector3Int candidatePivot = candidate->prefab.pivot();
					const spk::Vector3Int destination = mateWall - (mate->at - candidatePivot);
					const Claim box{
						.min = destination + candidate->prefab.minBounds() - candidatePivot,
						.max = destination + candidate->prefab.maxBounds() - candidatePivot};
					const bool insideSlot = box.min.x >= slotBounds.min.x && box.max.x <= slotBounds.max.x &&
											box.min.z >= slotBounds.min.z && box.max.z <= slotBounds.max.z;
					const bool collides = std::ranges::any_of(roomBoxes, [&](const Claim &p_placed) {
						return claimsOverlap(box, p_placed);
					});
					if (!insideSlot || collides)
					{
						continue;
					}

					placeRoom(*candidate, destination);
					// The rooms abut wall against wall: carve the two facing wall blocks
					// (body height, two blocks tall) so the shared doorway opens up.
					plan.placements.push_back(
						{.prefabId = connector.direction.x != 0 ? "interior-doorway-x" : "interior-doorway-z",
						 .anchor = {std::min(connector.at.x, mateWall.x), connector.at.y, std::min(connector.at.z, mateWall.z)},
						 .orientation = spk::VoxelOrientation::PositiveZ,
						 .foundation = false,
						 .anchorToPivot = true});
					// Both halves of the joint are connected now; neither may host another room.
					std::erase_if(open, [&](const OpenConnector &p_open) {
						return (p_open.at == connector.at && p_open.direction == connector.direction) ||
							   (p_open.at == mateWall && p_open.direction == inwardDirection);
					});
					++extraPlaced;
				}

				const spk::Vector3Int entryPivot = entryRoom->prefab.pivot();
				plan.portals.push_back({.from = doorWorld, .to = entryDestination + entryPad->at - entryPivot});
				plan.portals.push_back({.from = entryDestination + exitPad->at - entryPivot, .to = doorWorld + outward});
				++plan.stats.interiorCount;
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
					const std::string prefabId = pickPrefab(*pool);
					const spk::Vector3Int baseAnchor{
						offset + entity.col * blocks + blocks / 2,
						plan.surfaceY(level) + 1,
						offset + entity.row * blocks + blocks / 2};

					// The stairways claimed their zones first: the building only commits
					// where its own claimed zone is empty, nudging around the cell center
					// when the exact center is blocked.
					const std::array<spk::Vector3Int, 9> nudges = {
						spk::Vector3Int{0, 0, 0},
						{1, 0, 0},
						{-1, 0, 0},
						{0, 0, 1},
						{0, 0, -1},
						{2, 0, 0},
						{-2, 0, 0},
						{0, 0, 2},
						{0, 0, -2}};
					std::optional<PrefabPlacement> chosen;
					std::optional<Claim> chosenClaim;
					for (const spk::Vector3Int &nudge : nudges)
					{
						PrefabPlacement candidate{
							.prefabId = prefabId,
							.anchor = baseAnchor + nudge,
							.orientation = spk::VoxelOrientation::PositiveZ,
							.foundation = true};
						const std::optional<Claim> box = claimBoxFor(candidate);
						if (!box.has_value())
						{
							break; // unknown prefab: the provider will warn when stamping
						}
						if (zoneIsFree(*box))
						{
							chosen = std::move(candidate);
							chosenClaim = box;
							break;
						}
					}
					if (!chosen.has_value())
					{
						++plan.stats.placementConflicts;
						const bool vital = entity.kind == PlanEntityKind::Gym ||
										   entity.kind == PlanEntityKind::City ||
										   entity.kind == PlanEntityKind::PortCity;
						if (!vital)
						{
							// Plain POIs yield to whatever claimed the zone before them.
							++plan.stats.skippedPoiPlacements;
							continue;
						}
						// Progression buildings must exist: keep the center spot and let
						// the report call the overlap out.
						chosen = PrefabPlacement{
							.prefabId = prefabId,
							.anchor = baseAnchor,
							.orientation = spk::VoxelOrientation::PositiveZ,
							.foundation = true};
						chosenClaim = claimBoxFor(*chosen);
						plan.stats.warnings.push_back(
							"'" + prefabId + "' at cell (" + std::to_string(entity.row) + ", " +
							std::to_string(entity.col) + ") overlaps a prior claim (kept: progression building)");
					}
					plan.placements.push_back(*chosen);
					if (chosenClaim.has_value())
					{
						hardClaims.push_back(*chosenClaim);
					}
					composeInterior(plan.placements.back());
				}
			}

			// Scenery is biome dressing rather than a world entity. Density is the expected
			// number of instances requested per suitable plan cell; Poisson sampling keeps
			// fractional densities meaningful while allowing values above one. Placements
			// receive random voxel-level offsets and honor each prefab's center spacing.
			void placeScenery()
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
				for (int i = 0; i + 1 < size; ++i)
				{
					for (int j = 0; j + 1 < size; ++j)
					{
						if (plan.road.at(i, j) != 0 && plan.road.at(i, j + 1) != 0 &&
							plan.road.at(i + 1, j) != 0 && plan.road.at(i + 1, j + 1) != 0)
						{
							++stats.roadSquares;
						}
					}
				}
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
				placeScenery();
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
			biome.roadStairLengths = definition.worldgen->roadStairLengths;
			biome.roadStairPlatform = definition.worldgen->roadStairPlatform;
			biome.wildSlopeLengths = definition.worldgen->wildSlopeLengths;
			biome.wildSlopePlatform = definition.worldgen->wildSlopePlatform;
			for (const BiomeScenery &scenery : definition.worldgen->scenery)
			{
				biome.scenery.push_back({.prefabId = scenery.prefabId, .density = scenery.density, .spacing = scenery.spacing, .prefabSize = scenery.prefabSize});
			}
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
		out << "road 2x2 squares..... " << stats.roadSquares << "  (MUST be 0)\n";
		out << "boat links........... " << stats.boatLinks << "\n";
		out << "logically connected.. " << ((stats.roadComponents - stats.boatLinks) <= 1 ? "true" : "FALSE") << "  (want true)\n";
		out << "gateways (primary)... " << stats.primaryGateways << "\n";
		out << "gateways (secondary). " << stats.secondaryGateways << "\n";
		out << "stair prefabs........ " << stats.stairPlacements << "\n";
		out << "wild stair prefabs... " << stats.wildStairPlacements << " (" << wildStairs.size() << " stairways)\n";
		out << "composed stairways... " << stats.composedStairPlacements << "\n";
		out << "rejected stairways... " << stats.rejectedStairways << "\n";
		out << "scenery prefabs...... " << stats.sceneryPlacements << "\n";
		out << "interiors composed... " << stats.interiorCount << " (" << stats.interiorRoomPlacements
			<< " rooms, " << portals.size() << " portals)\n";
		out << "placement conflicts.. " << stats.placementConflicts << " (" << stats.skippedPoiPlacements
			<< " POIs skipped)\n";
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
		const PlanPlacementRules &p_placementRules,
		const Registry<PrefabDefinition> &p_prefabs,
		const Registry<InteriorDefinition> &p_interiors)
	{
		// This is the public allocation boundary. Keep validation ahead of Generator's
		// plan-grid construction and every arithmetic-heavy generation stage.
		validateWorldGenConfig(p_config);
		return Generator(p_config, p_biomes, p_placementRules, p_prefabs, p_interiors).run();
	}
}
