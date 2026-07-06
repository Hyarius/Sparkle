#include "world/generator/procedural_world.hpp"

#include "voxel/voxel_enums.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace
{
	constexpr int RouteStep = 8;
	constexpr std::array<pg::PlanCell, 4> Cardinal{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
	constexpr std::array<pg::PlanCell, 8> RouteNeighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};

	std::uint64_t splitMix64(std::uint64_t p_value) noexcept
	{
		p_value += 0x9e3779b97f4a7c15ULL;
		p_value = (p_value ^ (p_value >> 30)) * 0xbf58476d1ce4e5b9ULL;
		p_value = (p_value ^ (p_value >> 27)) * 0x94d049bb133111ebULL;
		return p_value ^ (p_value >> 31);
	}

	std::uint32_t stageSeed(std::uint64_t p_seed, std::uint64_t p_stage) noexcept
	{
		return static_cast<std::uint32_t>(splitMix64(p_seed ^ (p_stage * 0x9e3779b97f4a7c15ULL)));
	}

	std::uint64_t cellKey(pg::PlanCell p_cell) noexcept
	{
		return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(p_cell.x)) << 32) |
			   static_cast<std::uint32_t>(p_cell.y);
	}

	float distanceSquared(pg::PlanCell p_a, pg::PlanCell p_b) noexcept
	{
		const float dx = static_cast<float>(p_a.x - p_b.x);
		const float dy = static_cast<float>(p_a.y - p_b.y);
		return dx * dx + dy * dy;
	}

	std::vector<pg::PlanCell> cardinalLine(pg::PlanCell p_from, pg::PlanCell p_to)
	{
		std::vector<pg::PlanCell> result{p_from};
		int x = p_from.x;
		int y = p_from.y;
		const int dx = std::abs(p_to.x - p_from.x);
		const int sx = p_from.x < p_to.x ? 1 : -1;
		const int dy = -std::abs(p_to.y - p_from.y);
		const int sy = p_from.y < p_to.y ? 1 : -1;
		int error = dx + dy;
		while (x != p_to.x || y != p_to.y)
		{
			const int previousX = x;
			const int previousY = y;
			const int doubled = error * 2;
			if (doubled >= dy)
			{
				error += dy;
				x += sx;
			}
			if (doubled <= dx)
			{
				error += dx;
				y += sy;
			}
			if (x != previousX && y != previousY)
			{
				result.push_back({x, previousY});
			}
			result.push_back({x, y});
		}
		return result;
	}

	std::int8_t slopeOrientation(pg::PlanCell p_high, pg::PlanCell p_low)
	{
		if (p_low.x > p_high.x)
		{
			return static_cast<std::int8_t>(pg::VoxelOrientation::NegativeX);
		}
		if (p_low.x < p_high.x)
		{
			return static_cast<std::int8_t>(pg::VoxelOrientation::PositiveX);
		}
		if (p_low.y > p_high.y)
		{
			return static_cast<std::int8_t>(pg::VoxelOrientation::NegativeZ);
		}
		return static_cast<std::int8_t>(pg::VoxelOrientation::PositiveZ);
	}
}

namespace pg
{
	ProceduralWorld::ProceduralWorld(const WorldgenParams &p_params, std::uint64_t p_seed) :
		_params(p_params),
		_seed(p_seed),
		_warpX({.seed = stageSeed(p_seed, 1), .octaves = 2, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 0.0023f}),
		_warpY({.seed = stageSeed(p_seed, 2), .octaves = 2, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 0.0021f}),
		_coast({.seed = stageSeed(p_seed, 3), .octaves = 4, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = p_params.landmass.noiseFrequency}),
		_detail({.seed = stageSeed(p_seed, 4), .octaves = 2, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = p_params.landmass.detailFrequency}),
		_baseHeight({.seed = stageSeed(p_seed, 5), .octaves = 4, .persistence = 0.52f, .lacunarity = 2.0f, .frequency = p_params.height.baseFrequency}),
		_mountains({.seed = stageSeed(p_seed, 6), .octaves = 3, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = p_params.height.mountainFrequency * 0.27f}),
		_temperature({.seed = stageSeed(p_seed, 7), .octaves = 3, .persistence = 0.5f, .lacunarity = 2.0f, .frequency = 0.0035f}),
		_moisture({.seed = stageSeed(p_seed, 8), .octaves = 3, .persistence = 0.52f, .lacunarity = 2.0f, .frequency = 0.0045f})
	{
	}

	ProceduralWorld ProceduralWorld::generate(const WorldgenParams &p_params, std::uint64_t p_seed)
	{
		p_params.validate();
		ProceduralWorld result(p_params, p_seed);
		result._buildContinents();
		result._buildCities();
		result._buildRoads();
		return result;
	}

	int ProceduralWorld::width() const noexcept
	{
		return _params.worldSize[0];
	}

	int ProceduralWorld::height() const noexcept
	{
		return _params.worldSize[1];
	}

	std::uint64_t ProceduralWorld::seed() const noexcept
	{
		return _seed;
	}

	bool ProceduralWorld::contains(PlanCell p_cell) const noexcept
	{
		return p_cell.x >= 0 && p_cell.y >= 0 && p_cell.x < width() && p_cell.y < height();
	}

	const WorldgenParams &ProceduralWorld::params() const noexcept
	{
		return _params;
	}

	const std::vector<ProceduralContinent> &ProceduralWorld::continents() const noexcept
	{
		return _continents;
	}

	const std::vector<ProceduralCity> &ProceduralWorld::cities() const noexcept
	{
		return _cities;
	}

	const std::vector<ProceduralRoad> &ProceduralWorld::roads() const noexcept
	{
		return _roads;
	}

	void ProceduralWorld::_buildContinents()
	{
		std::mt19937 random(stageSeed(_seed, 20));
		std::uniform_int_distribution<int> countDistribution(
			_params.landmass.landmassCount[0], _params.landmass.landmassCount[1]);
		const int count = countDistribution(random);
		const float w = static_cast<float>(width());
		const float h = static_cast<float>(height());
		std::vector<std::array<float, 2>> centers;
		if (count == 1)
		{
			centers = {{{0.5f, 0.5f}}};
		}
		else if (count == 2)
		{
			centers = {{{0.30f, 0.42f}}, {{0.71f, 0.58f}}};
		}
		else
		{
			centers = {{{0.28f, 0.29f}}, {{0.73f, 0.32f}}, {{0.50f, 0.73f}}};
			for (int i = 3; i < count; ++i)
			{
				const float angle = static_cast<float>(i) * 6.283185307f / static_cast<float>(count);
				centers.push_back({0.5f + std::cos(angle) * 0.32f, 0.5f + std::sin(angle) * 0.32f});
			}
		}
		std::uniform_real_distribution<float> jitter(-0.025f, 0.025f);
		std::uniform_real_distribution<float> shapeX(0.78f, 1.20f);
		std::uniform_real_distribution<float> shapeY(0.74f, 1.16f);
		std::uniform_real_distribution<float> rotation(-0.45f, 0.45f);
		const float areaScale = count == 1 ? 0.37f : (count == 2 ? 0.255f : 0.205f);
		for (int i = 0; i < count; ++i)
		{
			_continents.push_back({.centerX = (centers[static_cast<std::size_t>(i)][0] + jitter(random)) * w, .centerY = (centers[static_cast<std::size_t>(i)][1] + jitter(random)) * h, .radiusX = areaScale * w * shapeX(random), .radiusY = areaScale * h * shapeY(random), .rotation = rotation(random)});
		}
	}

	std::pair<float, int> ProceduralWorld::_continentField(PlanCell p_cell) const
	{
		const float x = static_cast<float>(p_cell.x);
		const float y = static_cast<float>(p_cell.y);
		const float warpedX = x + _warpX.raw2D(x, y) * 62.0f;
		const float warpedY = y + _warpY.raw2D(x, y) * 62.0f;
		float best = -std::numeric_limits<float>::infinity();
		int bestContinent = -1;
		for (std::size_t i = 0; i < _continents.size(); ++i)
		{
			const ProceduralContinent &continent = _continents[i];
			const float cosine = std::cos(continent.rotation);
			const float sine = std::sin(continent.rotation);
			const float dx = warpedX - continent.centerX;
			const float dy = warpedY - continent.centerY;
			const float localX = cosine * dx + sine * dy;
			const float localY = -sine * dx + cosine * dy;
			const float angle = std::atan2(localY / continent.radiusY, localX / continent.radiusX);
			const float phase = static_cast<float>(
				static_cast<double>(stageSeed(_seed, 100 + i)) /
				static_cast<double>(std::numeric_limits<std::uint32_t>::max()) * 6.283185307);
			const float lobes = std::sin(angle * 3.0f + phase) * 0.085f +
								std::sin(angle * 5.0f - phase * 0.7f) * 0.045f;
			const float ellipse = 1.0f + lobes - std::sqrt(localX * localX / (continent.radiusX * continent.radiusX) + localY * localY / (continent.radiusY * continent.radiusY));
			if (ellipse > best)
			{
				best = ellipse;
				bestContinent = static_cast<int>(i);
			}
		}
		best += _coast.raw2D(x, y) * 0.29f + _detail.raw2D(x, y) * 0.075f;
		const float borderDistance = static_cast<float>(std::min({p_cell.x, p_cell.y, width() - 1 - p_cell.x, height() - 1 - p_cell.y}));
		if (borderDistance < static_cast<float>(_params.landmass.borderOceanWidth))
		{
			best -= (static_cast<float>(_params.landmass.borderOceanWidth) - borderDistance) /
					static_cast<float>(_params.landmass.borderOceanWidth);
		}
		return {best, bestContinent};
	}

	ProceduralTerrainSample ProceduralWorld::sampleTerrain(PlanCell p_cell) const
	{
		ProceduralTerrainSample result;
		if (!contains(p_cell))
		{
			return result;
		}
		const auto [field, continent] = _continentField(p_cell);
		result.continentalness = field;
		result.continent = field > 0.0f ? continent : -1;
		result.land = field > 0.0f;
		if (!result.land)
		{
			return result;
		}

		const float x = static_cast<float>(p_cell.x);
		const float y = static_cast<float>(p_cell.y);
		const float base = _baseHeight.sample2D(x, y);
		const float mountain = std::pow(
			std::clamp(std::abs(_mountains.raw2D(x, y)) * 2.5f, 0.0f, 1.0f), 1.7f);
		const float coastRamp = std::clamp(field / 0.16f, 0.0f, 1.0f);
		const float elevation = std::clamp((0.08f + base * 0.47f + mountain * 0.62f) * coastRamp, 0.0f, 1.0f);
		result.height = 1 + static_cast<int>(std::lround(
								std::pow(elevation, 1.12f) * static_cast<float>(_params.height.maxHeight - 1)));

		const float latitude = y / static_cast<float>(std::max(1, height() - 1));
		const float equator = 1.0f - std::abs(latitude * 2.0f - 1.0f);
		result.temperature = std::clamp(
			0.1f + equator * 0.8f + _temperature.raw2D(x, y) * 0.2f -
				static_cast<float>(result.height) / static_cast<float>(_params.height.maxHeight) * 0.38f,
			0.0f,
			1.0f);
		result.moisture = std::clamp(
			0.42f + _moisture.raw2D(x, y) * 0.38f + (1.0f - std::clamp(field / 0.45f, 0.0f, 1.0f)) * 0.16f,
			0.0f,
			1.0f);

		if (field < 0.055f)
		{
			result.biome = "coast";
		}
		else if (result.height >= static_cast<int>(_params.height.maxHeight * 0.88f) && result.temperature > 0.38f)
		{
			result.biome = "volcano";
		}
		else if (result.height >= static_cast<int>(_params.height.maxHeight * 0.68f))
		{
			result.biome = "highland";
		}
		else if (result.temperature < 0.31f)
		{
			result.biome = "tundra";
		}
		else if (result.moisture > 0.70f && result.height < static_cast<int>(_params.height.maxHeight * 0.28f))
		{
			result.biome = "swamp";
		}
		else if (result.temperature > 0.60f && result.moisture < 0.43f)
		{
			result.biome = "desert";
		}
		else if (result.moisture > 0.50f)
		{
			result.biome = "forest";
		}
		else
		{
			result.biome = "meadow";
		}
		return result;
	}

	ProceduralColumnSample ProceduralWorld::sample(PlanCell p_cell) const
	{
		const ProceduralTerrainSample terrain = sampleTerrain(p_cell);
		ProceduralColumnSample result;
		static_cast<ProceduralTerrainSample &>(result) = terrain;
		const auto road = _roadLookup.find(cellKey(p_cell));
		if (road != _roadLookup.end())
		{
			result.road = true;
			result.roadHeight = road->second.height;
			result.roadSlopeOrientation = road->second.slopeOrientation;
			result.height = result.roadHeight;
		}
		return result;
	}

	int ProceduralWorld::surfaceHeight(PlanCell p_cell) const
	{
		return sample(p_cell).height;
	}

	PlanCell ProceduralWorld::spawnCell() const
	{
		if (_cities.empty())
		{
			throw std::runtime_error("procedural world contains no city");
		}
		return _cities.front().cell;
	}

	void ProceduralWorld::_buildCities()
	{
		std::mt19937 random(stageSeed(_seed, 21));
		std::uniform_int_distribution<int> xDistribution(_params.landmass.borderOceanWidth, width() - 1 - _params.landmass.borderOceanWidth);
		std::uniform_int_distribution<int> yDistribution(_params.landmass.borderOceanWidth, height() - 1 - _params.landmass.borderOceanWidth);
		const float minimumDistanceSquared = _params.cities.minSpacing * _params.cities.minSpacing;
		const auto acceptable = [&](PlanCell p_cell, int p_requiredContinent, bool p_port) {
			const ProceduralTerrainSample sample = sampleTerrain(p_cell);
			if (!sample.land ||
				(p_requiredContinent >= 0 && sample.continent != p_requiredContinent))
			{
				return false;
			}
			if ((p_port && (sample.biome != "coast" || sample.continentalness < 0.025f)) ||
				(!p_port && sample.continentalness < 0.07f))
			{
				return false;
			}
			int minimum = sample.height;
			int maximum = sample.height;
			int landNeighbours = 0;
			for (const PlanCell offset : RouteNeighbours)
			{
				const ProceduralTerrainSample neighbour = sampleTerrain({p_cell.x + offset.x * 2, p_cell.y + offset.y * 2});
				if (!neighbour.land)
				{
					if (!p_port)
					{
						return false;
					}
					continue;
				}
				++landNeighbours;
				minimum = std::min(minimum, neighbour.height);
				maximum = std::max(maximum, neighbour.height);
			}
			if (maximum - minimum > 3 || (p_port && landNeighbours < 5))
			{
				return false;
			}
			return std::ranges::none_of(_cities, [&](const ProceduralCity &p_city) {
				return distanceSquared(p_city.cell, p_cell) < minimumDistanceSquared;
			});
		};

		const auto place = [&](int p_continent, bool p_port, auto &p_random) {
			for (int attempt = 0; attempt < 200000; ++attempt)
			{
				const PlanCell candidate{xDistribution(p_random), yDistribution(p_random)};
				if (!acceptable(candidate, p_continent, p_port))
				{
					continue;
				}
				const ProceduralTerrainSample sample = sampleTerrain(candidate);
				_cities.push_back({.id = static_cast<std::uint32_t>(_cities.size()), .cell = candidate, .continent = sample.continent, .biome = sample.biome, .port = p_port});
				return true;
			}
			return false;
		};

		for (std::size_t continent = 0; continent < _continents.size() &&
										_cities.size() < static_cast<std::size_t>(_params.cities.majorCount);
			 ++continent)
		{
			if (!place(static_cast<int>(continent), true, random))
			{
				throw std::runtime_error("could not place a coastal port town on every continent");
			}
		}
		while (_cities.size() < static_cast<std::size_t>(_params.cities.majorCount))
		{
			if (!place(-1, false, random))
			{
				throw std::runtime_error("could not place requested procedural cities");
			}
		}
	}

	std::vector<PlanCell> ProceduralWorld::_routeLand(PlanCell p_start, PlanCell p_goal, int p_continent) const
	{
		const int gridWidth = (width() + RouteStep - 1) / RouteStep;
		const int gridHeight = (height() + RouteStep - 1) / RouteStep;
		const auto gridIndex = [=](int p_x, int p_y) {
			return static_cast<std::size_t>(p_y) * static_cast<std::size_t>(gridWidth) + static_cast<std::size_t>(p_x);
		};
		const auto worldCell = [&](int p_x, int p_y) {
			return PlanCell{
				std::clamp(p_x * RouteStep + RouteStep / 2, 0, width() - 1),
				std::clamp(p_y * RouteStep + RouteStep / 2, 0, height() - 1)};
		};
		const int startX = std::clamp(p_start.x / RouteStep, 0, gridWidth - 1);
		const int startY = std::clamp(p_start.y / RouteStep, 0, gridHeight - 1);
		const int goalX = std::clamp(p_goal.x / RouteStep, 0, gridWidth - 1);
		const int goalY = std::clamp(p_goal.y / RouteStep, 0, gridHeight - 1);
		const std::size_t count = static_cast<std::size_t>(gridWidth) * static_cast<std::size_t>(gridHeight);
		std::vector<float> cost(count, std::numeric_limits<float>::infinity());
		std::vector<std::int32_t> parent(count, -1);
		using QueueItem = std::pair<float, std::size_t>;
		std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> pending;
		const std::size_t start = gridIndex(startX, startY);
		const std::size_t goal = gridIndex(goalX, goalY);
		cost[start] = 0.0f;
		pending.push({0.0f, start});
		while (!pending.empty())
		{
			const auto [estimated, index] = pending.top();
			pending.pop();
			(void)estimated;
			if (index == goal)
			{
				break;
			}
			const int x = static_cast<int>(index % static_cast<std::size_t>(gridWidth));
			const int y = static_cast<int>(index / static_cast<std::size_t>(gridWidth));
			const ProceduralTerrainSample currentSample = sampleTerrain(worldCell(x, y));
			for (const PlanCell offset : RouteNeighbours)
			{
				const int nextX = x + offset.x;
				const int nextY = y + offset.y;
				if (nextX < 0 || nextY < 0 || nextX >= gridWidth || nextY >= gridHeight)
				{
					continue;
				}
				const PlanCell nextCell = worldCell(nextX, nextY);
				const ProceduralTerrainSample nextSample = sampleTerrain(nextCell);
				if (!nextSample.land || nextSample.continent != p_continent)
				{
					continue;
				}
				const std::size_t next = gridIndex(nextX, nextY);
				const float slope = static_cast<float>(std::abs(nextSample.height - currentSample.height));
				const float coastPenalty = nextSample.continentalness < 0.08f ? 18.0f : 0.0f;
				const float stepCost = offset.x != 0 && offset.y != 0 ? 1.41421356f : 1.0f;
				const float candidate = cost[index] + stepCost + slope * 1.7f + coastPenalty;
				if (candidate >= cost[next])
				{
					continue;
				}
				cost[next] = candidate;
				parent[next] = static_cast<std::int32_t>(index);
				const int heuristicX = std::abs(nextX - goalX);
				const int heuristicY = std::abs(nextY - goalY);
				const float heuristic = static_cast<float>(std::max(heuristicX, heuristicY)) +
										0.41421356f * static_cast<float>(std::min(heuristicX, heuristicY));
				pending.push({candidate + heuristic, next});
			}
		}
		if (start != goal && parent[goal] < 0)
		{
			return cardinalLine(p_start, p_goal);
		}
		std::vector<PlanCell> coarse;
		for (std::size_t current = goal;; current = static_cast<std::size_t>(parent[current]))
		{
			const int x = static_cast<int>(current % static_cast<std::size_t>(gridWidth));
			const int y = static_cast<int>(current / static_cast<std::size_t>(gridWidth));
			coarse.push_back(worldCell(x, y));
			if (current == start)
			{
				break;
			}
		}
		std::ranges::reverse(coarse);
		std::vector<PlanCell> result;
		const auto append = [&](const std::vector<PlanCell> &p_cells, auto &p_result) {
			for (const PlanCell cell : p_cells)
			{
				if (p_result.empty() || p_result.back() != cell)
				{
					p_result.push_back(cell);
				}
			}
		};
		append(cardinalLine(p_start, coarse.front()), result);
		for (std::size_t i = 1; i < coarse.size(); ++i)
		{
			append(cardinalLine(coarse[i - 1], coarse[i]), result);
		}
		append(cardinalLine(coarse.back(), p_goal), result);
		return result;
	}

	void ProceduralWorld::_indexRoad(ProceduralRoad &p_road)
	{
		if (p_road.classification == TransportClass::Sea || p_road.cells.empty())
		{
			return;
		}
		std::vector<int> heights;
		heights.reserve(p_road.cells.size());
		for (const ProceduralRoadCell &cell : p_road.cells)
		{
			heights.push_back(sampleTerrain(cell.cell).height);
		}
		for (int pass = 0; pass < 8; ++pass)
		{
			for (std::size_t i = 1; i < heights.size(); ++i)
			{
				heights[i] = std::clamp(heights[i], heights[i - 1] - 1, heights[i - 1] + 1);
			}
			for (std::size_t i = heights.size(); i > 1; --i)
			{
				heights[i - 2] = std::clamp(heights[i - 2], heights[i - 1] - 1, heights[i - 1] + 1);
			}
		}
		for (std::size_t i = 0; i < p_road.cells.size(); ++i)
		{
			p_road.cells[i].height = heights[i];
			if (i > 0 && heights[i] != heights[i - 1])
			{
				const std::size_t high = heights[i] > heights[i - 1] ? i : i - 1;
				const std::size_t low = high == i ? i - 1 : i;
				p_road.cells[high].slopeOrientation = slopeOrientation(
					p_road.cells[high].cell, p_road.cells[low].cell);
			}
			_roadLookup[cellKey(p_road.cells[i].cell)] = {
				.height = p_road.cells[i].height,
				.slopeOrientation = p_road.cells[i].slopeOrientation};
		}
	}

	void ProceduralWorld::_buildRoads()
	{
		if (_cities.empty())
		{
			return;
		}
		std::vector<std::pair<std::size_t, std::size_t>> edges;
		std::vector<std::vector<std::size_t>> continentCities(_continents.size());
		std::vector<std::size_t> ports(_continents.size(), _cities.size());
		for (std::size_t city = 0; city < _cities.size(); ++city)
		{
			const std::size_t continent = static_cast<std::size_t>(_cities[city].continent);
			continentCities[continent].push_back(city);
			if (_cities[city].port)
			{
				ports[continent] = city;
			}
		}
		for (std::size_t continent = 0; continent < continentCities.size(); ++continent)
		{
			if (ports[continent] >= _cities.size())
			{
				throw std::runtime_error("continent is missing its port town");
			}
			const std::vector<std::size_t> &members = continentCities[continent];
			std::vector<bool> connected(members.size(), false);
			const auto portMember = std::ranges::find(members, ports[continent]);
			connected[static_cast<std::size_t>(std::distance(members.begin(), portMember))] = true;
			for (std::size_t count = 1; count < members.size(); ++count)
			{
				float best = std::numeric_limits<float>::infinity();
				std::pair<std::size_t, std::size_t> selected{};
				std::size_t selectedMember = 0;
				for (std::size_t a = 0; a < members.size(); ++a)
				{
					for (std::size_t b = 0; b < members.size(); ++b)
					{
						const float distance = distanceSquared(_cities[members[a]].cell, _cities[members[b]].cell);
						if (connected[a] && !connected[b] && distance < best)
						{
							best = distance;
							selected = {members[a], members[b]};
							selectedMember = b;
						}
					}
				}
				edges.push_back(selected);
				connected[selectedMember] = true;
			}
		}

		std::vector<bool> connectedContinents(_continents.size(), false);
		connectedContinents.front() = true;
		for (std::size_t count = 1; count < _continents.size(); ++count)
		{
			float best = std::numeric_limits<float>::infinity();
			std::pair<std::size_t, std::size_t> selected{};
			std::size_t selectedContinent = 0;
			for (std::size_t a = 0; a < _continents.size(); ++a)
			{
				for (std::size_t b = 0; b < _continents.size(); ++b)
				{
					const float distance = distanceSquared(_cities[ports[a]].cell, _cities[ports[b]].cell);
					if (connectedContinents[a] && !connectedContinents[b] && distance < best)
					{
						best = distance;
						selected = {ports[a], ports[b]};
						selectedContinent = b;
					}
				}
			}
			edges.push_back(selected);
			connectedContinents[selectedContinent] = true;
		}

		for (int extra = 0; extra < _params.transport.extraEdges; ++extra)
		{
			float best = std::numeric_limits<float>::infinity();
			std::pair<std::size_t, std::size_t> selected{_cities.size(), _cities.size()};
			for (std::size_t a = 0; a < _cities.size(); ++a)
			{
				for (std::size_t b = a + 1; b < _cities.size(); ++b)
				{
					std::pair<std::size_t, std::size_t> candidate = _cities[a].continent == _cities[b].continent
																		? std::pair{a, b}
																		: std::pair{
																			  ports[static_cast<std::size_t>(_cities[a].continent)],
																			  ports[static_cast<std::size_t>(_cities[b].continent)]};
					if (candidate.second < candidate.first)
					{
						std::swap(candidate.first, candidate.second);
					}
					const bool exists = std::ranges::any_of(edges, [&](const auto &p_edge) {
						return p_edge == candidate || p_edge == std::pair{candidate.second, candidate.first};
					});
					const float distance = distanceSquared(_cities[candidate.first].cell, _cities[candidate.second].cell);
					if (!exists && distance < best)
					{
						best = distance;
						selected = candidate;
					}
				}
			}
			if (selected.first < _cities.size())
			{
				edges.push_back(selected);
			}
		}

		for (const auto [from, to] : edges)
		{
			ProceduralRoad road;
			road.id = static_cast<std::uint32_t>(_roads.size());
			road.fromCity = static_cast<std::uint32_t>(from);
			road.toCity = static_cast<std::uint32_t>(to);
			road.classification = _cities[from].continent == _cities[to].continent
									  ? TransportClass::Road
									  : TransportClass::Sea;
			if (road.classification == TransportClass::Sea)
			{
				road.controlPoints = {_cities[from].cell, _cities[to].cell};
			}
			else
			{
				std::vector<PlanCell> path = _routeLand(
					_cities[from].cell, _cities[to].cell, _cities[from].continent);
				for (std::size_t i = 0; i < path.size(); i += 16)
				{
					road.controlPoints.push_back(path[i]);
				}
				if (road.controlPoints.empty() || road.controlPoints.back() != path.back())
				{
					road.controlPoints.push_back(path.back());
				}
				road.cells.reserve(path.size());
				for (const PlanCell cell : path)
				{
					if (sampleTerrain(cell).land)
					{
						road.cells.push_back({.cell = cell});
					}
				}
			}
			_roads.push_back(std::move(road));
			_indexRoad(_roads.back());
		}
	}

	std::size_t ProceduralWorld::manifestByteSize() const noexcept
	{
		std::size_t result = sizeof(_seed) + sizeof(WorldgenParams);
		result += _continents.size() * sizeof(ProceduralContinent);
		for (const ProceduralCity &city : _cities)
		{
			result += sizeof(ProceduralCity) + city.biome.size();
		}
		for (const ProceduralRoad &road : _roads)
		{
			result += sizeof(ProceduralRoad) + road.controlPoints.size() * sizeof(PlanCell) +
					  road.cells.size() * sizeof(ProceduralRoadCell);
		}
		return result;
	}

	std::uint64_t ProceduralWorld::hash() const noexcept
	{
		std::uint64_t result = splitMix64(_seed);
		const auto mix = [&](std::uint64_t p_value, auto &p_result) {
			p_result = splitMix64(p_result ^ p_value);
		};
		mix(static_cast<std::uint64_t>(width()), result);
		mix(static_cast<std::uint64_t>(height()), result);
		for (const ProceduralContinent &continent : _continents)
		{
			mix(std::bit_cast<std::uint32_t>(continent.centerX), result);
			mix(std::bit_cast<std::uint32_t>(continent.centerY), result);
			mix(std::bit_cast<std::uint32_t>(continent.radiusX), result);
			mix(std::bit_cast<std::uint32_t>(continent.radiusY), result);
		}
		for (const ProceduralCity &city : _cities)
		{
			mix(cellKey(city.cell), result);
			mix(static_cast<std::uint64_t>(city.continent), result);
			mix(city.port ? 1ULL : 0ULL, result);
		}
		for (const ProceduralRoad &road : _roads)
		{
			mix(road.fromCity, result);
			mix(road.toCity, result);
			mix(static_cast<std::uint8_t>(road.classification), result);
			for (const ProceduralRoadCell &cell : road.cells)
			{
				mix(cellKey(cell.cell), result);
				mix(static_cast<std::uint64_t>(cell.height), result);
			}
		}
		return result;
	}
}
