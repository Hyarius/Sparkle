#include "world/generator/plan_debug_image.hpp"

#include "world/generator/macro_world_plan.hpp"

#include <stb_image_write.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
	using Color = std::array<std::uint8_t, 3>;

	Color biomeColor(const std::string &p_biome)
	{
		if (p_biome == "forest")
		{
			return {58, 126, 72};
		}
		if (p_biome == "desert")
		{
			return {202, 164, 78};
		}
		if (p_biome == "tundra")
		{
			return {184, 207, 220};
		}
		if (p_biome == "swamp")
		{
			return {61, 103, 82};
		}
		if (p_biome == "volcano")
		{
			return {142, 67, 57};
		}
		if (p_biome == "meadow")
		{
			return {137, 181, 87};
		}
		if (p_biome == "coast")
		{
			return {219, 202, 139};
		}
		if (p_biome == "highland")
		{
			return {119, 122, 116};
		}
		return {128, 128, 128};
	}

	void paint(std::vector<std::uint8_t> &p_pixels, int p_width, int p_height, pg::PlanCell p_cell, Color p_color, int p_radius = 0)
	{
		for (int dy = -p_radius; dy <= p_radius; ++dy)
		{
			for (int dx = -p_radius; dx <= p_radius; ++dx)
			{
				if (dx * dx + dy * dy > p_radius * p_radius + 1)
				{
					continue;
				}
				const int x = p_cell.x + dx;
				const int y = p_cell.y + dy;
				if (x < 0 || y < 0 || x >= p_width || y >= p_height)
				{
					continue;
				}
				const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(p_width) + static_cast<std::size_t>(x)) * 3;
				std::copy(p_color.begin(), p_color.end(), p_pixels.begin() + static_cast<std::ptrdiff_t>(offset));
			}
		}
	}

	void darken(std::vector<std::uint8_t> &p_pixels, int p_width, pg::PlanCell p_cell, float p_factor)
	{
		const std::size_t offset =
			(static_cast<std::size_t>(p_cell.y) * static_cast<std::size_t>(p_width) + static_cast<std::size_t>(p_cell.x)) * 3;
		for (std::size_t channel = 0; channel < 3; ++channel)
		{
			p_pixels[offset + channel] = static_cast<std::uint8_t>(
				static_cast<float>(p_pixels[offset + channel]) * p_factor);
		}
	}
}

namespace pg
{
	void PlanDebugImage::writePng(const MacroWorldPlan &p_plan, const std::filesystem::path &p_output)
	{
		if (p_plan.width <= 0 || p_plan.height <= 0)
		{
			throw std::invalid_argument("cannot dump an empty macro world plan");
		}
		std::vector<std::uint8_t> pixels(static_cast<std::size_t>(p_plan.width) * static_cast<std::size_t>(p_plan.height) * 3);
		float maxHeight = 1.0f;
		for (const float value : p_plan.heights)
		{
			maxHeight = std::max(maxHeight, value);
		}
		for (int y = 0; y < p_plan.height; ++y)
		{
			for (int x = 0; x < p_plan.width; ++x)
			{
				const PlanCell cell{x, y};
				const std::size_t index = p_plan.index(cell);
				if (!p_plan.landMask[index])
				{
					paint(pixels, p_plan.width, p_plan.height, cell, {35, 83, 128});
					continue;
				}
				const std::size_t biome = p_plan.biomeCells[index] < 0 ? 0 : static_cast<std::size_t>(p_plan.biomeCells[index]);
				const std::string biomeId = biome < p_plan.biomeIds.size() ? p_plan.biomeIds[biome] : std::string{};
				Color color = biomeColor(biomeId);
				const float shade = 0.68f + 0.32f * p_plan.heights[index] / maxHeight;
				for (std::uint8_t &channel : color)
				{
					channel = static_cast<std::uint8_t>(static_cast<float>(channel) * shade);
				}
				paint(pixels, p_plan.width, p_plan.height, cell, color);
			}
		}

		constexpr int ContourInterval = 5;
		constexpr std::array<PlanCell, 4> CardinalNeighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
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
				const int heightBand = static_cast<int>(p_plan.heights[index]) / ContourInterval;
				const bool contour = std::ranges::any_of(CardinalNeighbours, [&](PlanCell p_offset) {
					const PlanCell neighbour{cell.x + p_offset.x, cell.y + p_offset.y};
					if (!p_plan.contains(neighbour))
					{
						return false;
					}
					const std::size_t neighbourIndex = p_plan.index(neighbour);
					return p_plan.landMask[neighbourIndex] &&
						   heightBand > static_cast<int>(p_plan.heights[neighbourIndex]) / ContourInterval;
				});
				if (contour)
				{
					darken(pixels, p_plan.width, cell, 0.42f);
				}
			}
		}

		for (const PlanRiver &river : p_plan.rivers)
		{
			for (const PlanCell cell : river.cells)
			{
				paint(pixels, p_plan.width, p_plan.height, cell, {51, 155, 222});
			}
		}
		for (const PlanRoad &road : p_plan.roads)
		{
			for (const PlanCell cell : road.cells)
			{
				paint(pixels, p_plan.width, p_plan.height, cell, {222, 196, 132});
			}
		}
		for (const PlanFeature &feature : p_plan.features)
		{
			Color color{255, 255, 255};
			if (feature.type == PlanFeatureType::Bridge)
			{
				color = {235, 99, 45};
			}
			else if (feature.type == PlanFeatureType::Port)
			{
				color = {255, 229, 71};
			}
			else if (feature.type == PlanFeatureType::Tunnel)
			{
				color = {199, 91, 217};
			}
			paint(pixels, p_plan.width, p_plan.height, feature.cell, color, 2);
		}
		for (const PlanSettlement &settlement : p_plan.settlements)
		{
			paint(pixels, p_plan.width, p_plan.height, settlement.cell, {245, 245, 245}, 2);
		}
		for (const PlanCity &city : p_plan.cities)
		{
			paint(pixels, p_plan.width, p_plan.height, city.cell, {22, 22, 25}, 4);
			paint(pixels, p_plan.width, p_plan.height, city.cell, {250, 244, 215}, 2);
		}

		if (!p_output.parent_path().empty())
		{
			std::filesystem::create_directories(p_output.parent_path());
		}
		if (stbi_write_png(p_output.string().c_str(), p_plan.width, p_plan.height, 3, pixels.data(), p_plan.width * 3) == 0)
		{
			throw std::runtime_error("failed to write macro world debug PNG: " + p_output.string());
		}
	}
}
