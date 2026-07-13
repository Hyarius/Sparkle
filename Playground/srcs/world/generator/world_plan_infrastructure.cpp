#include "world/generator/world_plan_generator.hpp"
#include "world/generator/town_commit.hpp"
#include "world/generator/town_planner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <optional>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

// The world's human layer: gateways between zones, settlements and POIs, port
// promotion, the road network with its boat links and bridges, and the buildings
// stamped at entity cells.
namespace pg::worldgen
{
	// ---------------- Stage C bis: gateways ----------------
	void Generator::resolveGateways()
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
	void Generator::placeEntities()
	{
		// Reserve scarce waterfronts before ordinary zones can occupy their configured
		// spacing radius across a biome border.
		std::vector<const PlanZone *> zoneOrder;
		zoneOrder.reserve(plan.zones.size());
		for (const PlanZone &zone : plan.zones) zoneOrder.push_back(&zone);
		std::stable_sort(zoneOrder.begin(), zoneOrder.end(), [&](const PlanZone *p_first, const PlanZone *p_second) {
			return plan.biomes[p_first->biomeIndex].requiresPort && !plan.biomes[p_second->biomeIndex].requiresPort;
		});
		for (const PlanZone *zonePointer : zoneOrder)
		{
			const PlanZone &zone = *zonePointer;
			const PlanBiome &biome = plan.biomes[zone.biomeIndex];
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
			const double area = static_cast<double>(cells.size());
			const double spacingArea = biome.townDensityDistanceCells * biome.townDensityDistanceCells;
			const double derivedTownCount = std::ceil(area / spacingArea);
			if (derivedTownCount > static_cast<double>(WorldGenConfig::MaximumPerZoneCount))
			{
				throw std::runtime_error(
					"zone " + std::to_string(zone.id) + " biome '" + biome.id + "' derives " +
					std::to_string(static_cast<int>(derivedTownCount)) +
					" towns; increase worldgen.towns.densityDistanceCells");
			}
			const int requiredTowns = 1 + (biome.requiresPort ? 1 : 0); // every biome has a gym
			const int townTarget = std::max(requiredTowns, static_cast<int>(derivedTownCount));
			const auto isSettlement = [](PlanEntityKind p_kind) {
				return p_kind == PlanEntityKind::Gym || p_kind == PlanEntityKind::City ||
					p_kind == PlanEntityKind::PortCity;
			};
			const auto settlementDistanceFor = [&](int p_zone) {
				return plan.biomes[plan.zones[p_zone].biomeIndex].minimumTownDistanceCells;
			};
			const auto placementOk = [&](int p_row, int p_col, PlanEntityKind p_kind, double p_block, bool p_enforceTownDistance) {
				const bool candidateIsSettlement = isSettlement(p_kind);
				for (const PlanEntity &entity : plan.entities)
				{
					const double distance = std::hypot(p_row - entity.row, p_col - entity.col);
					if (distance == 0.0)
					{
						return false;
					}
					if (distance < p_block)
					{
						return false;
					}
					if (p_enforceTownDistance && candidateIsSettlement && isSettlement(entity.kind) &&
						distance < std::max(biome.minimumTownDistanceCells, settlementDistanceFor(entity.zone)))
					{
						return false;
					}
				}
				return true;
			};
			const auto nearestSettlementDistance = [&](const Cell &p_candidate) {
				double result = INF;
				for (const PlanEntity &entity : plan.entities)
				{
					if (isSettlement(entity.kind))
					{
						result = std::min(result, std::hypot(p_candidate.row - entity.row, p_candidate.col - entity.col));
					}
				}
				return result;
			};

			enum class CoastRule
			{
				Any,
				Coastal
			};
			const auto sample = [&](PlanEntityKind p_kind, int p_count, double p_block, CoastRule p_rule, bool p_enforceTownDistance = true) {
				int got = 0;
				std::vector<Cell> order = cells;
				rng.shuffle(order);
				while (got < p_count)
				{
					std::optional<Cell> selected;
					double bestNearestSettlement = -1.0;
					for (const Cell &candidate : order)
					{
						const double dCoast = distOcean.at(candidate.row, candidate.col);
						if (p_rule == CoastRule::Coastal && dCoast > cfg.coastDistCells) continue;
						if (!placementOk(candidate.row, candidate.col, p_kind, p_block, p_enforceTownDistance)) continue;
						if (!isSettlement(p_kind))
						{
							selected = candidate;
							break;
						}
						const double nearest = nearestSettlementDistance(candidate);
						if (!selected || nearest > bestNearestSettlement)
						{
							selected = candidate;
							bestNearestSettlement = nearest;
						}
					}
					if (!selected) break;
					plan.entities.push_back({.kind = p_kind, .row = selected->row, .col = selected->col, .zone = zone.id, .continent = zoneContinent.contains(zone.id) ? zoneContinent[zone.id] : 1});
					++got;
				}
				return got;
			};

			// Port and gym are mandatory settlement roles. Place the waterfront role first
			// so its scarce candidates cannot be consumed by an ordinary city or gym.
			if (biome.requiresPort && sample(PlanEntityKind::PortCity, 1, cfg.blockCity, CoastRule::Coastal) != 1)
			{
				throw std::runtime_error(
					"zone " + std::to_string(zone.id) + " biome '" + biome.id + "'" +
					" requires a port but has no coastal settlement site");
			}
			int gyms = sample(PlanEntityKind::Gym, 1, cfg.blockGym, CoastRule::Any);
			if (gyms == 0) gyms = sample(PlanEntityKind::Gym, 1, 0.0, CoastRule::Any, false);
			if (gyms != 1) throw std::logic_error("a placeable biome zone could not reserve its mandatory gym");

			const int requestedCities = townTarget - requiredTowns;
			const int cities = sample(PlanEntityKind::City, requestedCities, cfg.blockCity, CoastRule::Any);
			if (cities < requestedCities)
			{
				plan.stats.warnings.push_back(
					"zone " + std::to_string(zone.id) + ": settlement target " + std::to_string(cities + requiredTowns) +
					"/" + std::to_string(townTarget) + " after preserving configured minimum town distance");
			}

			sample(PlanEntityKind::RarePoi, cfg.rarePoiPerZone, cfg.blockRare, CoastRule::Any);
			sample(PlanEntityKind::UncommonPoi, cfg.uncommonPoiPerZone, cfg.blockUncommon, CoastRule::Any);
			sample(PlanEntityKind::NormalPoi, cfg.normalPoiPerZone, cfg.blockNormal, CoastRule::Any);
		}
	}

	void Generator::reserveTownSites()
	{
		for (std::size_t index = 0; index < plan.entities.size(); ++index)
		{
			PlanEntity &entity = plan.entities[index];
			if (entity.kind != PlanEntityKind::City && entity.kind != PlanEntityKind::Gym && entity.kind != PlanEntityKind::PortCity)
				continue;
			const TownCompositionKind wanted = entity.kind == PlanEntityKind::Gym ? TownCompositionKind::Gym : entity.kind == PlanEntityKind::PortCity ? TownCompositionKind::Port : TownCompositionKind::City;
			const std::vector<std::string> ids = townCompositions.ids();
			const auto compatible = std::find_if(ids.begin(), ids.end(), [&](const std::string &id) { return townCompositions.get(id).kind == wanted; });
			if (compatible == ids.end())
				throw std::runtime_error("town site reservation failed: no compatible composition for settlement kind");
			const TownComposition &composition = townCompositions.get(*compatible);
			const PlanTown *biomeTown = townFor(entity.zone);
			if (biomeTown == nullptr)
				throw TownPlanningError(index, *compatible, {.category = TownRejectCategory::MissingContent, .componentId = *compatible, .message = "settlement biome has no town content"}, {}, "town site reservation cannot resolve biome town content");
			TownRejection sizingFailure;
			const std::optional<int> resolvedRadius = deriveTownRadius(composition, prefabs, *biomeTown, cfg.masterSeed, index, sizingFailure);
			if (!resolvedRadius)
				throw TownPlanningError(index, *compatible, sizingFailure, {{sizingFailure.componentId, 1}}, "town site reservation cannot derive a composition envelope: " + sizingFailure.message);
			const int townRadius = *resolvedRadius;
			const auto hasDryFootprint = [&](int p_row, int p_col, int p_radiusColumns) {
				const int centerX = plan.worldOffset() + p_col * cfg.blocksPerCell + cfg.blocksPerCell / 2;
				const int centerZ = plan.worldOffset() + p_row * cfg.blocksPerCell + cfg.blocksPerCell / 2;
				const int firstRow = plan.cellIndexFromWorld(centerZ - p_radiusColumns);
				const int lastRow = plan.cellIndexFromWorld(centerZ + p_radiusColumns);
				const int firstCol = plan.cellIndexFromWorld(centerX - p_radiusColumns);
				const int lastCol = plan.cellIndexFromWorld(centerX + p_radiusColumns);
				for (int row = firstRow; row <= lastRow; ++row)
					for (int col = firstCol; col <= lastCol; ++col)
						if (!isLand(row, col) || plan.water.at(row, col) != 0 || plan.zone.at(row, col) != entity.zone) return false;
				return true;
			};
			const auto hasWaterfront = [&](int p_row, int p_col) {
				const int centerX = plan.worldOffset() + p_col * cfg.blocksPerCell + cfg.blocksPerCell / 2;
				const int centerZ = plan.worldOffset() + p_row * cfg.blocksPerCell + cfg.blocksPerCell / 2;
				const int firstRow = plan.cellIndexFromWorld(centerZ - townRadius);
				const int lastRow = plan.cellIndexFromWorld(centerZ + townRadius);
				const int firstCol = plan.cellIndexFromWorld(centerX - townRadius);
				const int lastCol = plan.cellIndexFromWorld(centerX + townRadius);
				for (int row = firstRow; row <= lastRow; ++row)
					for (int col = firstCol; col <= lastCol; ++col)
						if (!isLand(row, col) || plan.water.at(row, col) != 0) return true;
				return false;
			};
			// The entity becomes stable at reservation time, before roads are built.
			// The envelope is resolved from the seeded contents before roads are built.
			const int envelope = (townRadius + cfg.blocksPerCell - 1) / cfg.blocksPerCell;
			std::optional<Cell> reservedCenter;
			// Prefer the sampled marker, then search farther within the same affiliated
			// biome. A town never relocates into a neighboring biome just because that
			// biome has a more convenient dry envelope.
			for (int pass = 0; pass < (entity.kind == PlanEntityKind::PortCity ? 1 : 2) && !reservedCenter; ++pass)
			{
				const int searchRadius = pass == 0 ? cfg.townSearchRadiusCells : size;
				for (int radius = 0; radius <= searchRadius && !reservedCenter; ++radius)
				{
					for (int dr = -radius; dr <= radius && !reservedCenter; ++dr)
					{
						for (int dc = -radius; dc <= radius; ++dc)
						{
							if (std::max(std::abs(dr), std::abs(dc)) != radius) continue;
							const int row = entity.row + dr, col = entity.col + dc;
							if (row < 0 || col < 0 || row >= size || col >= size || row - envelope < 0 || col - envelope < 0 || row + envelope >= size || col + envelope >= size || plan.zone.at(row, col) != entity.zone) continue;
							// Building foundations can safely terrace height changes at voxel scale,
							// but no local layout can make a required building dry if its reservation
							// already spans water. Normal towns therefore require a dry full envelope.
							// Ports retain one outer macro-cell ring for their dock while preserving a
							// dry central construction core.
							if (!isLand(row, col) || plan.water.at(row, col) != 0) continue;
							const int dryRadius = entity.kind == PlanEntityKind::PortCity ? std::max(0, townRadius - cfg.blocksPerCell) : townRadius;
							if (!hasDryFootprint(row, col, dryRadius)) continue;
							if (entity.kind == PlanEntityKind::PortCity && !hasWaterfront(row, col)) continue;
							bool separated = true;
							for (std::size_t otherIndex = 0; otherIndex < plan.entities.size(); ++otherIndex)
							{
								if (otherIndex == index) continue;
								const PlanEntity &other = plan.entities[otherIndex];
								const bool otherSettlement = other.kind == PlanEntityKind::City || other.kind == PlanEntityKind::Gym || other.kind == PlanEntityKind::PortCity;
								// Town workspaces are inclusive squares.  Manhattan distance permits
								// diagonal overlap and a one-column seam at exactly two radii, so
								// reserve a strictly disjoint Chebyshev envelope instead.
								if (otherSettlement && std::max(std::abs(other.row - row), std::abs(other.col - col)) <= envelope * 2) { separated = false; break; }
							}
							if (separated) reservedCenter = Cell{row, col};
						}
					}
				}
			}
			if (!reservedCenter)
				throw TownPlanningError(index, *compatible, {.category = TownRejectCategory::SiteTerrain, .componentId = *compatible, .message = entity.kind == PlanEntityKind::PortCity ? "no dry waterfront reservation envelope in the affiliated biome" : "no dry affiliated-biome reservation envelope"}, {}, "town site reservation cannot find an affiliated-biome foundation envelope");
			entity.row = reservedCenter->row;
			entity.col = reservedCenter->col;
			const int centerX = plan.worldOffset() + entity.col * cfg.blocksPerCell + cfg.blocksPerCell / 2;
			const int centerZ = plan.worldOffset() + entity.row * cfg.blocksPerCell + cfg.blocksPerCell / 2;
			const int worldMaximum = plan.worldOffset() + cfg.size * cfg.blocksPerCell - 1;
			if (centerX - townRadius < plan.worldOffset() || centerX + townRadius > worldMaximum ||
				centerZ - townRadius < plan.worldOffset() || centerZ + townRadius > worldMaximum ||
				!isLand(entity.row, entity.col) || plan.water.at(entity.row, entity.col) != 0)
			{
				throw TownPlanningError(index, *compatible, {.category = TownRejectCategory::SiteOutOfBounds, .componentId = *compatible, .message = "settlement center cannot reserve the composition boundary"}, {}, "town site reservation is outside dry world bounds");
			}
			plan.townSites.push_back({.macroEntityIndex = index, .kind = entity.kind, .compositionId = *compatible, .centerRow = entity.row, .centerCol = entity.col, .radiusColumns = townRadius});
		}
	}

	// ---------------- Stage F: roads ----------------
	// Roads stay one cell wide: a cell may not be laid if it would fill a 2x2 block
	// of road (four road units meeting in a square). Because riding on top of an
	// existing road is cheaper than running beside it, blocking the square just makes
	// parallel paths merge onto the same trunk instead of thickening it. Checks the
	// four 2x2 squares that contain (p_row, p_col) against the roads stamped so far.
	bool Generator::wouldFormRoadSquare(int p_row, int p_col) const
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

	double Generator::stepCost(int p_fromRow, int p_fromCol, int p_row, int p_col, const Cell &p_goal) const
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
	std::vector<Cell> Generator::findPath(const Cell &p_start, const Cell &p_goal)
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

	bool Generator::connectRoad(const Cell &p_from, const Cell &p_to)
	{
		const std::vector<Cell> path = findPath(p_from, p_to);
		for (const Cell &cell : path)
		{
			plan.road.at(cell.row, cell.col) = 1;
		}
		return !path.empty();
	}

	std::optional<Cell> Generator::nearestRoadCell(int p_row, int p_col, int p_maxRadius) const
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

	void Generator::buildRoads()
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

	void Generator::buildTownSpines()
	{
		// A settlement is already a road endpoint.  Continue its road through the
		// reserved town envelope before any building placement chooses an address.
		for (const PlanTownSite &site : plan.townSites)
		{
			const int cells = std::max(1, (site.radiusColumns + cfg.blocksPerCell - 1) / cfg.blocksPerCell);
			const std::array<Cell, 4> directions = {{{-1, 0}, {0, 1}, {1, 0}, {0, -1}}};
			std::vector<Cell> arrivals;
			for (Cell direction : directions)
			{
				const int row = site.centerRow + direction.row;
				const int col = site.centerCol + direction.col;
				if (plan.road.contains(row, col) && plan.road.at(row, col) != 0) arrivals.push_back(direction);
			}
			if (arrivals.empty()) arrivals.push_back({-1, 0});
			for (const Cell arrival : arrivals)
			{
				const Cell direction{-arrival.row, -arrival.col};
				for (int step = 1; step <= cells; ++step)
				{
					const int row = site.centerRow + direction.row * step;
					const int col = site.centerCol + direction.col * step;
					if (!plan.road.contains(row, col) || !isLand(row, col) || plan.water.at(row, col) != 0 ||
						(plan.road.at(row, col) == 0 && wouldFormRoadSquare(row, col))) break;
					plan.road.at(row, col) = 1;
				}
			}
			if (plan.road.at(site.centerRow, site.centerCol) != 0 || !wouldFormRoadSquare(site.centerRow, site.centerCol))
				plan.road.at(site.centerRow, site.centerCol) = 1;
		}
		// Town spines are added after the primary road pass, so run the same
		// square cleanup once more before the voxel-scale town planner projects it.
		removeRoadSquares();
	}

	// Guarantees roads stay one cell wide. The A* guard keeps parallel paths from
	// thickening, but the diagonal-elbow reconstruction and forced junctions can still
	// leave a stray 2x2 block; this erases the redundant corner of each one. A cell is
	// removed only when both of its neighbours outside the square are non-road, so its
	// sole links are the two square siblings, which stay connected through the diagonal
	// sibling (an L) -- nothing external can be cut off. True 4-way crossings, where
	// every corner has an outside road neighbour, are left untouched.
	void Generator::removeRoadSquares()
	{
		const auto isRoad = [&](int p_r, int p_c) {
			return p_r >= 0 && p_c >= 0 && p_r < size && p_c < size && plan.road.at(p_r, p_c) != 0;
		};
		const auto isTownCenter = [&](int p_row, int p_col) {
			return std::ranges::any_of(plan.townSites, [&](const PlanTownSite &site) {
				return site.centerRow == p_row && site.centerCol == p_col;
			});
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
						if (!isTownCenter(corner[0], corner[1]) && !isRoad(corner[2], corner[3]) && !isRoad(corner[4], corner[5]))
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

	void Generator::addBoatLinks()
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
	void Generator::markBridges()
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

	const std::string &Generator::pickPrefab(const std::vector<std::string> &p_pool)
	{
		return p_pool.size() == 1 ? p_pool.front() : p_pool[prefabPickRng.below(static_cast<int>(p_pool.size()))];
	}

	const std::vector<std::string> *Generator::entityPrefabsFor(PlanEntityKind p_kind, int p_zone) const
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

	const PlanTown *Generator::townFor(int p_zone) const
	{
		if (p_zone < 0 || p_zone >= static_cast<int>(plan.zones.size()))
		{
			return nullptr;
		}
		const PlanBiome &biome = plan.biomes[plan.zones[p_zone].biomeIndex];
		return biome.town.has_value() ? &*biome.town : nullptr;
	}

	void Generator::planAndCommitTowns()
	{
		// commitTownCandidate swaps a fully merged WorldPlan into place, so planning
		// must iterate a stable snapshot rather than references into plan.townSites.
		const std::vector<PlanTownSite> sites = plan.townSites;
		for (const PlanTownSite &site : sites)
		{
			if (site.macroEntityIndex >= plan.entities.size()) throw std::logic_error("town site has an invalid settlement owner");
			if (site.compositionId.empty()) throw std::logic_error("town site has no selected composition id");
			const PlanEntity &entity = plan.entities[site.macroEntityIndex];
			const PlanTown *town = townFor(entity.zone);
			if (town == nullptr) throw std::runtime_error("town planning failed: settlement biome has no town content");
			const TownComposition &composition = townCompositions.get(site.compositionId);
			TownPlanResult attempt = planTown(plan, site, composition, prefabs, *town, cfg.masterSeed);
			plan.stats.townLayoutAttempts += attempt.layoutAttempts;
			plan.stats.townBuildingCandidates += attempt.buildingCandidates;
			plan.stats.townRouteExpansions += attempt.routeExpansions;
			if (!attempt.candidate)
			{
				const TownRejection rejection = attempt.rejection.value_or(TownRejection{.category = TownRejectCategory::CommitInvariant, .componentId = composition.id, .message = "layout attempts exhausted"});
				throw TownPlanningError(site.macroEntityIndex, composition.id, rejection, {{rejection.componentId, 1}}, "town planning failed after bounded local layout attempts: " + rejection.message);
			}
			TownCandidate candidate = std::move(*attempt.candidate);
			if (entity.kind == PlanEntityKind::PortCity)
			{
				for (std::size_t link = 0; link < plan.boatLinks.size(); ++link)
				{
					const auto matches = [&](const PlanEntity &other) { return other.kind == entity.kind && other.row == entity.row && other.col == entity.col; };
					if (matches(plan.boatLinks[link].first) || matches(plan.boatLinks[link].second)) candidate.boatLinkIndices.push_back(link);
				}
			}
			commitTownCandidate(plan, candidate);
			for (const PlanClaim &claim : candidate.buildingClaims) hardClaims.push_back({claim.min, claim.max});
			for (const PlanClaim &claim : candidate.dockClaims) hardClaims.push_back({claim.min, claim.max});
			for (const PlanClaim &claim : candidate.sceneryClaims) hardClaims.push_back({claim.min, claim.max});
			// The exact town contents above are individual claims, but the whole town
			// rectangle is reserved from later world dressing. Without this umbrella
			// claim, an exterior tree or POI can fit in an intentionally open street.
			const TownWorldBounds &bounds = candidate.bounds;
			hardClaims.push_back(
				{{bounds.minX, std::numeric_limits<int>::lowest(), bounds.minZ},
				 {bounds.maxX, std::numeric_limits<int>::max(), bounds.maxZ}});
			for (std::size_t placement : plan.towns.back().buildingPlacementIndices) composeInterior(plan.placements[placement]);
		}
	}

	void Generator::placeBuildings()
	{
		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		for (std::size_t entityIndex=0; entityIndex<plan.entities.size(); ++entityIndex)
		{
			const PlanEntity &entity=plan.entities[entityIndex];
			if (entity.kind == PlanEntityKind::Gym || entity.kind == PlanEntityKind::City ||
				entity.kind == PlanEntityKind::PortCity)
			{
				// Settlements were planned transactionally after their global road spine.
				// This stage stamps only ordinary POIs and must not create a second town writer.
				continue;
			}
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

}
