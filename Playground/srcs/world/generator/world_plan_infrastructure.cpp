#include "world/generator/world_plan_generator.hpp"

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

	void Generator::assignPorts()
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

	void Generator::composeTown(const PlanEntity &p_entity)
	{
		const PlanTown *town = townFor(p_entity.zone);
		if (town == nullptr)
		{
			plan.stats.warnings.push_back("settlement has no town definition for its biome");
			return;
		}

		struct Building
		{
			TownBuildingRole role;
			std::string prefab;
			int rowOffset;
			int colOffset;
		};
		std::vector<Building> buildings = {
			// Current town prefabs expose their door on local -Z.  Keep every
			// building south of the hub so its road approaches that door side,
			// instead of drawing a decorative connection to a wall or its roof.
			{TownBuildingRole::CreatureCenter, town->creatureCenter, 2, -2},
			{TownBuildingRole::Shop, town->shop, 2, 2}};
		if (p_entity.kind == PlanEntityKind::Gym)
		{
			buildings.push_back({TownBuildingRole::Gym, town->gym, 4, 0});
		}
		if (p_entity.kind == PlanEntityKind::PortCity)
		{
			// The south edge is a stable first layout.  A later coastline-aware layout
			// can move this slot without changing the port role or travel binding.
			buildings.push_back({TownBuildingRole::Port, town->port, 4, 0});
		}
		const std::array<std::pair<int, int>, 4> homeSlots = {{{4, -3}, {4, -1}, {4, 1}, {4, 3}}};
		for (const auto &[rowOffset, colOffset] : homeSlots)
		{
			buildings.push_back({TownBuildingRole::Home, pickPrefab(town->homes), rowOffset, colOffset});
		}

		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		// A town path is only allowed over existing dry terrain.  In particular it
		// must route around rivers instead of turning their cells into road during
		// chunk realization.
		const auto dryPathTo = [&](int p_targetRow, int p_targetCol) {
			using PathCell = std::pair<int, int>;
			constexpr int LocalSearchRadius = 18;
			const auto passable = [&](int p_row, int p_col) {
				return plan.land.contains(p_row, p_col) && isLand(p_row, p_col) && plan.water.at(p_row, p_col) == 0 &&
					std::abs(p_row - p_entity.row) <= LocalSearchRadius &&
					std::abs(p_col - p_entity.col) <= LocalSearchRadius;
			};
			std::vector<PathCell> empty;
			const PathCell doorApproach{p_targetRow - 1, p_targetCol};
			if (!passable(p_targetRow, p_targetCol) || !passable(doorApproach.first, doorApproach.second) ||
				!passable(p_entity.row, p_entity.col))
			{
				return empty;
			}
			PlanGrid<std::uint8_t> visited(size, 0);
			PlanGrid<PathCell> previous(size, {-1, -1});
			std::queue<PathCell> open;
			open.push({p_entity.row, p_entity.col});
			visited.at(p_entity.row, p_entity.col) = 1;
			constexpr std::array<PathCell, 4> Neighbors = {{{-1, 0}, {0, -1}, {0, 1}, {1, 0}}};
			while (!open.empty())
			{
				const PathCell current = open.front();
				open.pop();
				if (current == doorApproach)
				{
					std::vector<PathCell> path;
					for (PathCell step = current; step != PathCell{p_entity.row, p_entity.col}; step = previous.at(step.first, step.second))
					{
						path.push_back(step);
					}
					path.push_back({p_entity.row, p_entity.col});
					std::ranges::reverse(path);
					path.push_back({p_targetRow, p_targetCol}); // the north-facing door strip
					return path;
				}
				for (const PathCell &delta : Neighbors)
				{
					const PathCell next{current.first + delta.first, current.second + delta.second};
					if (passable(next.first, next.second) && visited.at(next.first, next.second) == 0)
					{
						visited.at(next.first, next.second) = 1;
						previous.at(next.first, next.second) = current;
						open.push(next);
					}
				}
			}
			return empty;
		};
		const auto hasLevelDryFootprint = [&](const Claim &p_claim, int p_height) {
			const int minRow = plan.cellIndexFromWorld(p_claim.min.z);
			const int maxRow = plan.cellIndexFromWorld(p_claim.max.z);
			const int minCol = plan.cellIndexFromWorld(p_claim.min.x);
			const int maxCol = plan.cellIndexFromWorld(p_claim.max.x);
			for (int row = minRow; row <= maxRow; ++row)
			{
				for (int col = minCol; col <= maxCol; ++col)
				{
					if (!plan.land.contains(row, col) || !isLand(row, col) || plan.water.at(row, col) != 0 ||
						plan.height.at(row, col) != p_height)
					{
						return false;
					}
				}
			}
			return true;
		};

		// A town is a single candidate: collect every resolved lot, door route and
		// claim before changing the plan.  This is the first transactional boundary
		// of the blueprint system; partial villages are never committed.
		std::set<std::pair<int, int>> occupiedSlots;
		std::vector<PrefabPlacement> plannedBuildings;
		std::vector<Claim> plannedClaims;
		std::vector<std::pair<int, int>> plannedRoad;
		for (const Building &building : buildings)
		{
			std::vector<std::pair<int, int>> candidates = {{p_entity.row + building.rowOffset, p_entity.col + building.colOffset}};
			// Town entities can occur near a coast, river, or zone boundary. Search a
			// stable local area when the template slot is unusable; required services
			// are never simply dropped because their first slot is wet.
			for (int radius = 1; radius <= 16; ++radius)
			{
				for (int rowOffset = -radius; rowOffset <= radius; ++rowOffset)
				{
					for (int colOffset = -radius; colOffset <= radius; ++colOffset)
					{
						if (std::max(std::abs(rowOffset), std::abs(colOffset)) == radius)
						{
							candidates.emplace_back(p_entity.row + rowOffset, p_entity.col + colOffset);
						}
					}
				}
			}
			std::optional<PrefabPlacement> selected;
			std::optional<Claim> selectedClaim;
			std::pair<int, int> selectedCell;
			std::vector<std::pair<int, int>> selectedPath;
			for (const auto &[row, col] : candidates)
			{
				if (row <= p_entity.row || !plan.height.contains(row, col) || !isLand(row, col) || plan.water.at(row, col) != 0 ||
					occupiedSlots.contains({row, col}))
				{
					continue;
				}
				PrefabPlacement candidate{
					.prefabId = building.prefab,
					.anchor = {offset + col * blocks + blocks / 2, plan.surfaceY(plan.height.at(row, col)) + 1,
						offset + row * blocks + blocks / 2},
					.orientation = spk::VoxelOrientation::PositiveZ,
					.foundation = true,
					.townRole = building.role};
				const std::optional<Claim> claim = claimBoxFor(candidate);
				if (!claim.has_value() || !hasLevelDryFootprint(*claim, plan.height.at(row, col)) || !zoneIsFree(*claim) ||
					std::ranges::any_of(plannedClaims, [&](const Claim &p_planned) { return claimsOverlap(*claim, p_planned); }))
				{
					continue;
				}
				std::vector<std::pair<int, int>> path = dryPathTo(row, col);
				if (path.empty())
				{
					continue;
				}
				selected = std::move(candidate);
				selectedClaim = claim;
				selectedCell = {row, col};
				selectedPath = std::move(path);
				break;
			}
			if (!selected.has_value())
			{
				++plan.stats.placementConflicts;
				plan.stats.warnings.push_back("town blueprint rejected: building '" + building.prefab + "' has no valid lot");
				return;
			}
			occupiedSlots.insert(selectedCell);
			plannedRoad.insert(plannedRoad.end(), selectedPath.begin(), selectedPath.end());
			plannedBuildings.push_back(std::move(*selected));
			if (selectedClaim.has_value())
			{
				plannedClaims.push_back(*selectedClaim);
			}
		}

		for (const auto &[row, col] : plannedRoad)
		{
			plan.townRoad.at(row, col) = 1;
		}
		hardClaims.insert(hardClaims.end(), plannedClaims.begin(), plannedClaims.end());
		for (PrefabPlacement &building : plannedBuildings)
		{
			plan.placements.push_back(std::move(building));
			composeInterior(plan.placements.back());
		}
	}

	void Generator::placeBuildings()
	{
		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		for (const PlanEntity &entity : plan.entities)
		{
			if (entity.kind == PlanEntityKind::Gym || entity.kind == PlanEntityKind::City ||
				entity.kind == PlanEntityKind::PortCity)
			{
				composeTown(entity);
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

	void Generator::reserveTownAreas()
	{
		// Buildings already claimed their individual boxes.  Reserve the entire
		// connected town-road footprint as well, including its empty lots, so the
		// later scenery stage cannot put trees, rocks, or bushes in a village.
		PlanGrid<std::uint8_t> visited(size, 0);
		constexpr std::array<std::pair<int, int>, 4> neighbors = {{{-1, 0}, {0, -1}, {0, 1}, {1, 0}}};
		const int blocks = cfg.blocksPerCell;
		const int offset = plan.worldOffset();
		for (int startRow = 0; startRow < size; ++startRow)
		{
			for (int startCol = 0; startCol < size; ++startCol)
			{
				if (plan.townRoad.at(startRow, startCol) == 0 || visited.at(startRow, startCol) != 0)
				{
					continue;
				}
				int minRow = startRow;
				int maxRow = startRow;
				int minCol = startCol;
				int maxCol = startCol;
				int minSurface = plan.surfaceY(plan.height.at(startRow, startCol));
				int maxSurface = minSurface;
				std::queue<std::pair<int, int>> open;
				open.push({startRow, startCol});
				visited.at(startRow, startCol) = 1;
				while (!open.empty())
				{
					const auto [row, col] = open.front();
					open.pop();
					minRow = std::min(minRow, row);
					maxRow = std::max(maxRow, row);
					minCol = std::min(minCol, col);
					maxCol = std::max(maxCol, col);
					const int surface = plan.surfaceY(plan.height.at(row, col));
					minSurface = std::min(minSurface, surface);
					maxSurface = std::max(maxSurface, surface);
					for (const auto &[rowDelta, colDelta] : neighbors)
					{
						const int nextRow = row + rowDelta;
						const int nextCol = col + colDelta;
						if (plan.townRoad.contains(nextRow, nextCol) && plan.townRoad.at(nextRow, nextCol) != 0 &&
							visited.at(nextRow, nextCol) == 0)
						{
							visited.at(nextRow, nextCol) = 1;
							open.push({nextRow, nextCol});
						}
					}
				}
				hardClaims.push_back({
					.min = {offset + minCol * blocks, minSurface - 16, offset + minRow * blocks},
					.max = {offset + (maxCol + 1) * blocks - 1, maxSurface + 64, offset + (maxRow + 1) * blocks - 1}});
			}
		}
	}
}
