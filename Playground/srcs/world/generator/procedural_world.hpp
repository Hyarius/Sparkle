#pragma once

#include "structures/math/spk_perlin.hpp"
#include "world/generator/macro_world_plan.hpp"
#include "world/generator/worldgen_params.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace pg
{
	struct ProceduralContinent
	{
		float centerX = 0.0f;
		float centerY = 0.0f;
		float radiusX = 1.0f;
		float radiusY = 1.0f;
		float rotation = 0.0f;
	};

	struct ProceduralCity
	{
		std::uint32_t id = 0;
		PlanCell cell;
		int continent = -1;
		std::string biome;
		bool port = false;
	};

	struct ProceduralRoadCell
	{
		PlanCell cell;
		int height = 0;
		std::int8_t slopeOrientation = -1;
	};

	struct ProceduralRoad
	{
		std::uint32_t id = 0;
		std::uint32_t fromCity = 0;
		std::uint32_t toCity = 0;
		TransportClass classification = TransportClass::Road;
		std::vector<PlanCell> controlPoints;
		std::vector<ProceduralRoadCell> cells;
	};

	struct ProceduralTerrainSample
	{
		bool land = false;
		int continent = -1;
		float continentalness = 0.0f;
		int height = 0;
		float temperature = 0.0f;
		float moisture = 0.0f;
		std::string biome;
	};

	struct ProceduralColumnSample : ProceduralTerrainSample
	{
		bool road = false;
		int roadHeight = 0;
		std::int8_t roadSlopeOrientation = -1;
	};

	class ProceduralWorld
	{
	private:
		struct RoadLookup
		{
			int height = 0;
			std::int8_t slopeOrientation = -1;
		};

		WorldgenParams _params;
		std::uint64_t _seed = 0;
		spk::Perlin _warpX;
		spk::Perlin _warpY;
		spk::Perlin _coast;
		spk::Perlin _detail;
		spk::Perlin _baseHeight;
		spk::Perlin _mountains;
		spk::Perlin _temperature;
		spk::Perlin _moisture;
		std::vector<ProceduralContinent> _continents;
		std::vector<ProceduralCity> _cities;
		std::vector<ProceduralRoad> _roads;
		std::unordered_map<std::uint64_t, RoadLookup> _roadLookup;

		explicit ProceduralWorld(const WorldgenParams &p_params, std::uint64_t p_seed);
		[[nodiscard]] std::pair<float, int> _continentField(PlanCell p_cell) const;
		void _buildContinents();
		void _buildCities();
		void _buildRoads();
		[[nodiscard]] std::vector<PlanCell> _routeLand(PlanCell p_start, PlanCell p_goal, int p_continent) const;
		void _indexRoad(ProceduralRoad &p_road);

	public:
		[[nodiscard]] static ProceduralWorld generate(const WorldgenParams &p_params, std::uint64_t p_seed);

		[[nodiscard]] int width() const noexcept;
		[[nodiscard]] int height() const noexcept;
		[[nodiscard]] std::uint64_t seed() const noexcept;
		[[nodiscard]] bool contains(PlanCell p_cell) const noexcept;
		[[nodiscard]] ProceduralTerrainSample sampleTerrain(PlanCell p_cell) const;
		[[nodiscard]] ProceduralColumnSample sample(PlanCell p_cell) const;
		[[nodiscard]] int surfaceHeight(PlanCell p_cell) const;
		[[nodiscard]] PlanCell spawnCell() const;
		[[nodiscard]] const WorldgenParams &params() const noexcept;
		[[nodiscard]] const std::vector<ProceduralContinent> &continents() const noexcept;
		[[nodiscard]] const std::vector<ProceduralCity> &cities() const noexcept;
		[[nodiscard]] const std::vector<ProceduralRoad> &roads() const noexcept;
		[[nodiscard]] std::size_t manifestByteSize() const noexcept;
		[[nodiscard]] std::uint64_t hash() const noexcept;
	};
}
