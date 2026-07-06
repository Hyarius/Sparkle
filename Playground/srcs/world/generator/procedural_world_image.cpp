#include "world/generator/procedural_world_image.hpp"

#include "world/generator/procedural_world.hpp"

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
			return {44, 98, 80};
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
		for (int y = -p_radius; y <= p_radius; ++y)
		{
			for (int x = -p_radius; x <= p_radius; ++x)
			{
				if (x * x + y * y > p_radius * p_radius + 1)
				{
					continue;
				}
				const int targetX = p_cell.x + x;
				const int targetY = p_cell.y + y;
				if (targetX < 0 || targetY < 0 || targetX >= p_width || targetY >= p_height)
				{
					continue;
				}
				const std::size_t offset =
					(static_cast<std::size_t>(targetY) * static_cast<std::size_t>(p_width) + static_cast<std::size_t>(targetX)) * 3;
				std::copy(p_color.begin(), p_color.end(), p_pixels.begin() + static_cast<std::ptrdiff_t>(offset));
			}
		}
	}

	void drawLine(std::vector<std::uint8_t> &p_pixels, int p_width, int p_height, pg::PlanCell p_from, pg::PlanCell p_to, Color p_color)
	{
		int x = p_from.x;
		int y = p_from.y;
		const int dx = std::abs(p_to.x - p_from.x);
		const int sx = p_from.x < p_to.x ? 1 : -1;
		const int dy = -std::abs(p_to.y - p_from.y);
		const int sy = p_from.y < p_to.y ? 1 : -1;
		int error = dx + dy;
		while (true)
		{
			paint(p_pixels, p_width, p_height, {x, y}, p_color);
			if (x == p_to.x && y == p_to.y)
			{
				break;
			}
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
		}
	}
}

namespace pg
{
	void ProceduralWorldImage::writePng(const ProceduralWorld &p_world, const std::filesystem::path &p_output)
	{
		std::vector<std::uint8_t> pixels(
			static_cast<std::size_t>(p_world.width()) * static_cast<std::size_t>(p_world.height()) * 3);
		std::vector<ProceduralTerrainSample> samples(
			static_cast<std::size_t>(p_world.width()) * static_cast<std::size_t>(p_world.height()));
		const auto index = [&](int p_x, int p_y) {
			return static_cast<std::size_t>(p_y) * static_cast<std::size_t>(p_world.width()) + static_cast<std::size_t>(p_x);
		};
		for (int y = 0; y < p_world.height(); ++y)
		{
			for (int x = 0; x < p_world.width(); ++x)
			{
				const ProceduralTerrainSample sample = p_world.sampleTerrain({x, y});
				samples[index(x, y)] = sample;
				if (!sample.land)
				{
					paint(pixels, p_world.width(), p_world.height(), {x, y}, {35, 83, 128});
					continue;
				}
				Color color = biomeColor(sample.biome);
				const float shade = 0.70f + 0.30f * static_cast<float>(sample.height) /
												static_cast<float>(p_world.params().height.maxHeight);
				for (std::uint8_t &channel : color)
				{
					channel = static_cast<std::uint8_t>(static_cast<float>(channel) * shade);
				}
				paint(pixels, p_world.width(), p_world.height(), {x, y}, color);
			}
		}

		constexpr int ContourInterval = 5;
		constexpr std::array<PlanCell, 4> neighbours{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
		for (int y = 1; y + 1 < p_world.height(); ++y)
		{
			for (int x = 1; x + 1 < p_world.width(); ++x)
			{
				const ProceduralTerrainSample &sample = samples[index(x, y)];
				if (!sample.land)
				{
					continue;
				}
				const int band = sample.height / ContourInterval;
				const bool contour = std::ranges::any_of(neighbours, [&](PlanCell p_offset) {
					const ProceduralTerrainSample &neighbour = samples[index(x + p_offset.x, y + p_offset.y)];
					return neighbour.land && band > neighbour.height / ContourInterval;
				});
				if (!contour)
				{
					continue;
				}
				const std::size_t pixel = index(x, y) * 3;
				for (std::size_t channel = 0; channel < 3; ++channel)
				{
					pixels[pixel + channel] = static_cast<std::uint8_t>(static_cast<float>(pixels[pixel + channel]) * 0.43f);
				}
			}
		}

		for (const ProceduralRoad &road : p_world.roads())
		{
			if (road.classification == TransportClass::Sea)
			{
				for (std::size_t i = 1; i < road.controlPoints.size(); ++i)
				{
					drawLine(pixels, p_world.width(), p_world.height(), road.controlPoints[i - 1], road.controlPoints[i], {86, 181, 224});
				}
				continue;
			}
			for (const ProceduralRoadCell &cell : road.cells)
			{
				paint(pixels, p_world.width(), p_world.height(), cell.cell, {231, 206, 140});
			}
		}
		for (const ProceduralCity &city : p_world.cities())
		{
			paint(pixels, p_world.width(), p_world.height(), city.cell, {22, 22, 25}, city.port ? 7 : 5);
			paint(
				pixels,
				p_world.width(),
				p_world.height(),
				city.cell,
				city.port ? Color{255, 211, 65} : Color{250, 244, 215},
				city.port ? 5 : 3);
			if (city.port)
			{
				paint(pixels, p_world.width(), p_world.height(), city.cell, {61, 154, 211}, 2);
			}
		}

		if (!p_output.parent_path().empty())
		{
			std::filesystem::create_directories(p_output.parent_path());
		}
		if (stbi_write_png(
				p_output.string().c_str(), p_world.width(), p_world.height(), 3, pixels.data(), p_world.width() * 3) == 0)
		{
			throw std::runtime_error("failed to write procedural world PNG: " + p_output.string());
		}
	}
}
