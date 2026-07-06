#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct PlanCell
	{
		int x = 0;
		int y = 0;

		auto operator<=>(const PlanCell &) const = default;
	};

	enum class TransportClass : std::uint8_t
	{
		Road,
		Bridge,
		Sea,
		Tunnel
	};

	enum class PlanFeatureType : std::uint8_t
	{
		Bridge,
		Port,
		Tunnel
	};

	struct PlanCity
	{
		PlanCell cell;
		std::string biome;
		float biomeWeight = 1.0f;
	};

	struct PlanSettlement
	{
		PlanCell cell;
		std::size_t hubCity = 0;
		std::string biome;
	};

	struct PlanTransportEdge
	{
		// Node indices address cities first, followed by settlements.
		std::size_t from = 0;
		std::size_t to = 0;
		TransportClass classification = TransportClass::Road;
	};

	struct PlanRoad
	{
		std::size_t edge = 0;
		std::vector<PlanCell> cells;
	};

	struct PlanFeature
	{
		PlanFeatureType type = PlanFeatureType::Bridge;
		PlanCell cell;
		std::size_t edge = 0;
	};

	struct PlanRiver
	{
		std::vector<PlanCell> cells;
		bool reachesSea = false;
		bool endsInLakeBasin = false;
	};

	struct PlanInfo
	{
		bool land = false;
		bool ocean = false;
		int oceanDistance = 0;
		int macroRegion = -1;
		int landmass = -1;
		float height = 0.0f;
		std::string biome;
		bool road = false;
		bool river = false;
		std::optional<PlanFeatureType> feature;
	};

	class MacroWorldPlan
	{
	public:
		int width = 0;
		int height = 0;
		std::uint64_t runSeed = 0;
		std::vector<std::uint8_t> landMask;
		std::vector<std::uint8_t> oceanMask;
		std::vector<std::uint16_t> oceanDistances;
		std::vector<std::int32_t> macroRegionCells;
		std::vector<std::int16_t> landmassCells;
		std::vector<float> heights;
		std::vector<std::uint8_t> riverMask;
		std::vector<std::uint8_t> roadMask;
		std::vector<std::uint8_t> featureMask;
		std::vector<std::int16_t> biomeCells;
		std::vector<std::string> biomeIds;
		std::vector<PlanCity> cities;
		std::vector<PlanSettlement> settlements;
		std::vector<PlanTransportEdge> edges;
		std::vector<PlanRoad> roads;
		std::vector<PlanFeature> features;
		std::vector<PlanRiver> rivers;

		MacroWorldPlan() = default;
		MacroWorldPlan(int p_width, int p_height, std::uint64_t p_runSeed);

		[[nodiscard]] bool contains(PlanCell p_cell) const noexcept;
		[[nodiscard]] std::size_t index(PlanCell p_cell) const;
		[[nodiscard]] PlanCell nodeCell(std::size_t p_node) const;
		[[nodiscard]] PlanInfo queryCell(PlanCell p_cell) const;
		[[nodiscard]] std::uint64_t hash() const noexcept;
	};
}
