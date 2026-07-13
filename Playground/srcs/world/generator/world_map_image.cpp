#include "world/generator/world_map_image.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <stb_image_write.h>

// Self-contained raster renderer: RGB byte canvas + tiny 5x7 bitmap font + a handful of
// marker shapes, saved through stb_image_write (whose implementation Sparkle compiles).

namespace
{
	struct Color
	{
		std::uint8_t r = 0;
		std::uint8_t g = 0;
		std::uint8_t b = 0;
	};

	[[nodiscard]] Color fromHex(std::uint32_t p_rgb)
	{
		return {static_cast<std::uint8_t>((p_rgb >> 16) & 0xFF),
				static_cast<std::uint8_t>((p_rgb >> 8) & 0xFF),
				static_cast<std::uint8_t>(p_rgb & 0xFF)};
	}

	[[nodiscard]] Color fromSpk(const spk::Color &p_color)
	{
		const auto channel = [](float p_value) {
			return static_cast<std::uint8_t>(std::clamp(p_value, 0.0f, 1.0f) * 255.0f + 0.5f);
		};
		return {channel(p_color.r), channel(p_color.g), channel(p_color.b)};
	}

	[[nodiscard]] Color shade(const Color &p_color, double p_factor)
	{
		const auto scale = [&](std::uint8_t p_channel) {
			return static_cast<std::uint8_t>(std::clamp(p_channel * p_factor, 0.0, 255.0));
		};
		return {scale(p_color.r), scale(p_color.g), scale(p_color.b)};
	}

	struct Canvas
	{
		int width = 0;
		int height = 0;
		std::vector<std::uint8_t> pixels;

		Canvas(int p_width, int p_height, const Color &p_fill) : width(p_width), height(p_height), pixels()
		{
			pixels.resize(static_cast<std::size_t>(p_width) * p_height * 3);
			for (int y = 0; y < p_height; ++y)
			{
				for (int x = 0; x < p_width; ++x)
				{
					set(x, y, p_fill);
				}
			}
		}

		void set(int p_x, int p_y, const Color &p_color)
		{
			if (p_x < 0 || p_y < 0 || p_x >= width || p_y >= height)
			{
				return;
			}
			const std::size_t index = (static_cast<std::size_t>(p_y) * width + p_x) * 3;
			pixels[index] = p_color.r;
			pixels[index + 1] = p_color.g;
			pixels[index + 2] = p_color.b;
		}

		void fillRect(int p_x, int p_y, int p_width, int p_height, const Color &p_color)
		{
			for (int y = p_y; y < p_y + p_height; ++y)
			{
				for (int x = p_x; x < p_x + p_width; ++x)
				{
					set(x, y, p_color);
				}
			}
		}

		void fillCircle(int p_cx, int p_cy, int p_radius, const Color &p_color)
		{
			for (int y = -p_radius; y <= p_radius; ++y)
			{
				for (int x = -p_radius; x <= p_radius; ++x)
				{
					if (x * x + y * y <= p_radius * p_radius)
					{
						set(p_cx + x, p_cy + y, p_color);
					}
				}
			}
		}

		void fillDiamond(int p_cx, int p_cy, int p_radius, const Color &p_color)
		{
			for (int y = -p_radius; y <= p_radius; ++y)
			{
				for (int x = -p_radius; x <= p_radius; ++x)
				{
					if (std::abs(x) + std::abs(y) <= p_radius)
					{
						set(p_cx + x, p_cy + y, p_color);
					}
				}
			}
		}

		void fillPlus(int p_cx, int p_cy, int p_radius, int p_thickness, const Color &p_color)
		{
			fillRect(p_cx - p_thickness / 2, p_cy - p_radius, p_thickness, 2 * p_radius + 1, p_color);
			fillRect(p_cx - p_radius, p_cy - p_thickness / 2, 2 * p_radius + 1, p_thickness, p_color);
		}

		void strokeCross(int p_cx, int p_cy, int p_radius, const Color &p_color)
		{
			for (int step = -p_radius; step <= p_radius; ++step)
			{
				for (int thick = 0; thick < 2; ++thick)
				{
					set(p_cx + step, p_cy + step + thick, p_color);
					set(p_cx + step, p_cy - step + thick, p_color);
				}
			}
		}

		// Five-pointed star via even-odd point-in-polygon over its 10 vertices.
		void fillStar(int p_cx, int p_cy, double p_outer, double p_inner, const Color &p_color)
		{
			constexpr int PointCount = 10;
			double xs[PointCount];
			double ys[PointCount];
			for (int point = 0; point < PointCount; ++point)
			{
				const double radius = point % 2 == 0 ? p_outer : p_inner;
				const double angle = -3.14159265358979323846 / 2.0 + point * (3.14159265358979323846 / 5.0);
				xs[point] = p_cx + radius * std::cos(angle);
				ys[point] = p_cy + radius * std::sin(angle);
			}
			const int bound = static_cast<int>(p_outer) + 1;
			for (int y = -bound; y <= bound; ++y)
			{
				for (int x = -bound; x <= bound; ++x)
				{
					const double px = p_cx + x;
					const double py = p_cy + y;
					bool inside = false;
					for (int i = 0, j = PointCount - 1; i < PointCount; j = i++)
					{
						if (((ys[i] > py) != (ys[j] > py)) &&
							(px < (xs[j] - xs[i]) * (py - ys[i]) / (ys[j] - ys[i]) + xs[i]))
						{
							inside = !inside;
						}
					}
					if (inside)
					{
						set(p_cx + x, p_cy + y, p_color);
					}
				}
			}
		}

		void drawDashedLine(int p_x0, int p_y0, int p_x1, int p_y1, const Color &p_color)
		{
			const int dx = std::abs(p_x1 - p_x0);
			const int dy = -std::abs(p_y1 - p_y0);
			const int sx = p_x0 < p_x1 ? 1 : -1;
			const int sy = p_y0 < p_y1 ? 1 : -1;
			int error = dx + dy;
			int x = p_x0;
			int y = p_y0;
			int step = 0;
			while (true)
			{
				if ((step / 4) % 2 == 0)
				{
					set(x, y, p_color);
					set(x + 1, y, p_color);
				}
				if (x == p_x1 && y == p_y1)
				{
					break;
				}
				const int doubled = 2 * error;
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
				++step;
			}
		}
	};

	// 5x7 font, uppercase + digits + a few symbols; each glyph row is 5 bits, MSB left.
	struct Glyph
	{
		char character;
		std::uint8_t rows[7];
	};

	constexpr Glyph FontTable[] = {
		{'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
		{'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
		{'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
		{'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
		{'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
		{'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
		{'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
		{'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
		{'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
		{'J', {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}},
		{'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
		{'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
		{'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
		{'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
		{'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
		{'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
		{'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
		{'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
		{'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
		{'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
		{'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
		{'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
		{'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
		{'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
		{'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
		{'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
		{'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
		{'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
		{'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
		{'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
		{'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
		{'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
		{'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
		{'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
		{'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
		{'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
		{'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
		{'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
		{':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
		{' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
	};

	[[nodiscard]] const Glyph *findGlyph(char p_character)
	{
		const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(p_character)));
		for (const Glyph &glyph : FontTable)
		{
			if (glyph.character == upper)
			{
				return &glyph;
			}
		}
		return nullptr;
	}

	[[nodiscard]] int textWidth(const std::string &p_text, int p_scale)
	{
		return static_cast<int>(p_text.size()) * 6 * p_scale - p_scale;
	}

	void drawText(Canvas &p_canvas, int p_x, int p_y, const std::string &p_text, const Color &p_color, int p_scale)
	{
		int cursor = p_x;
		for (const char character : p_text)
		{
			const Glyph *glyph = findGlyph(character);
			if (glyph != nullptr)
			{
				for (int row = 0; row < 7; ++row)
				{
					for (int column = 0; column < 5; ++column)
					{
						if ((glyph->rows[row] & (1 << (4 - column))) != 0)
						{
							p_canvas.fillRect(cursor + column * p_scale, p_y + row * p_scale, p_scale, p_scale, p_color);
						}
					}
				}
			}
			cursor += 6 * p_scale;
		}
	}

	void drawLabelBox(Canvas &p_canvas, int p_centerX, int p_topY, const std::vector<std::string> &p_lines, int p_scale)
	{
		int widest = 0;
		for (const std::string &line : p_lines)
		{
			widest = std::max(widest, textWidth(line, p_scale));
		}
		const int lineHeight = 8 * p_scale;
		const int pad = 3 * p_scale;
		const int boxWidth = widest + 2 * pad;
		const int boxHeight = static_cast<int>(p_lines.size()) * lineHeight + 2 * pad - p_scale;
		p_canvas.fillRect(p_centerX - boxWidth / 2, p_topY, boxWidth, boxHeight, {245, 245, 235});
		for (std::size_t index = 0; index < p_lines.size(); ++index)
		{
			const int width = textWidth(p_lines[index], p_scale);
			drawText(
				p_canvas,
				p_centerX - width / 2,
				p_topY + pad + static_cast<int>(index) * lineHeight,
				p_lines[index],
				{20, 20, 20},
				p_scale);
		}
	}

	// Fallback zone fills when a biome does not name its own map color.
	constexpr std::uint32_t FallbackZoneColors[] = {
		0xE8743B, 0xF2C14E, 0x3FA34D, 0x8FBF3F, 0x2FA68A, 0x3E7CB1, 0xB0496B, 0x5FB0B7, 0x9B6BDF, 0xC0653B};
}

namespace pg
{
	bool writeWorldMapPng(const WorldPlan &p_plan, const std::filesystem::path &p_path)
	{
		const int size = p_plan.config.size;
		const int scale = 10;
		const int titleBar = 34;
		const Color ocean = fromHex(0x2C5F8A);
		Canvas canvas(size * scale, size * scale + titleBar, ocean);

		const auto zoneColor = [&](int p_zone) {
			const PlanBiome &biome = p_plan.biomes[p_plan.zones[p_zone].biomeIndex];
			if (biome.mapColor.has_value())
			{
				return fromSpk(*biome.mapColor);
			}
			return fromHex(FallbackZoneColors[p_zone % (sizeof(FallbackZoneColors) / sizeof(FallbackZoneColors[0]))]);
		};

		// Terrain: zone color shaded by strata level, then rivers/lakes on top.
		const Color riverColor = fromHex(0x9BD4EE);
		const Color lakeColor = fromHex(0x5FB0D8);
		for (int row = 0; row < size; ++row)
		{
			for (int col = 0; col < size; ++col)
			{
				Color color = ocean;
				if (p_plan.land.at(row, col) != 0)
				{
					const int zone = p_plan.zone.at(row, col);
					const int level = std::max<int>(0, p_plan.height.at(row, col));
					const double factor = 0.68 + 0.42 * (static_cast<double>(level) / std::max(1, p_plan.config.maxHeightLevel));
					color = zone >= 0 ? shade(zoneColor(zone), factor) : shade(fromHex(0x888888), factor);
					if (p_plan.lake.at(row, col) != 0)
					{
						color = lakeColor;
					}
					else if (p_plan.water.at(row, col) != 0)
					{
						color = riverColor;
					}
				}
				canvas.fillRect(col * scale, titleBar + row * scale, scale, scale, color);
			}
		}

		// Roads as centered dark squares, bridges as white squares (matches the previewer).
		const Color roadColor = fromHex(0x33302B);
		const Color bridgeColor = fromHex(0xF2F2F2);
		for (int row = 0; row < size; ++row)
		{
			for (int col = 0; col < size; ++col)
			{
				if (p_plan.road.at(row, col) == 0)
				{
					continue;
				}
				const Color &color = p_plan.bridge.at(row, col) != 0 ? bridgeColor : roadColor;
				canvas.fillRect(col * scale + scale / 2 - 2, titleBar + row * scale + scale / 2 - 2, 5, 5, color);
			}
		}
		// Resolved town ownership and exact streets come from the
		// committed records, never from connected-component inference.
		for (const PlanTownRecord &town : p_plan.towns)
		{
			for (const auto &[row,col] : town.boundaryCells) canvas.fillRect(col*scale+1,titleBar+row*scale+1,scale-2,scale-2,fromHex(0x7B4AA8));
			for (const PlanPavedColumn &road : town.mainRoadSurface) { const int row=p_plan.cellIndexFromWorld(road.worldZ), col=p_plan.cellIndexFromWorld(road.worldX); canvas.fillRect(col*scale+scale/2-2,titleBar+row*scale+scale/2-2,5,5,fromHex(0xD88A2D)); }
			for (const PlanPavedColumn &road : town.urbanRoadSurface) { const int row=p_plan.cellIndexFromWorld(road.worldZ), col=p_plan.cellIndexFromWorld(road.worldX); canvas.fillRect(col*scale+scale/2-1,titleBar+row*scale+scale/2-1,3,3,fromHex(0xFFD166)); }
			for(const auto &[row,col]:town.dockCells)canvas.fillRect(col*scale+2,titleBar+row*scale+2,scale-4,scale-4,fromHex(0x9A6A35));
			if(town.boardingEndpoint) { const int col=p_plan.cellIndexFromWorld(town.boardingEndpoint->x), row=p_plan.cellIndexFromWorld(town.boardingEndpoint->z); canvas.fillDiamond(col*scale+scale/2,titleBar+row*scale+scale/2,4,fromHex(0x2BE3E3)); }
		}

		// Stairways repaint the road line where a climb detours it: every committed
		// stair rectangle (flights, platforms, walkway lane) is drawn in road color at
		// sub-cell resolution, so the map shows the path the player actually walks.
		{
			const int blocks = p_plan.config.blocksPerCell;
			const int offset = p_plan.worldOffset();
			const auto toPixel = [&](int p_world) {
				return static_cast<int>((static_cast<long long>(p_world - offset) * scale) / blocks);
			};
			for (const PlanStairway &stairway : p_plan.stairways)
			{
				for (const PlanStairRect &rect : stairway.footprints)
				{
					const int x0 = toPixel(rect.minX);
					const int x1 = toPixel(rect.maxX + 1);
					const int z0 = toPixel(rect.minZ);
					const int z1 = toPixel(rect.maxZ + 1);
					canvas.fillRect(x0, titleBar + z0, std::max(1, x1 - x0), std::max(1, z1 - z0), roadColor);
				}
			}
		}

		// Wild staircases: brown square with a white border so they stand out from the
		// black road dots (they mark off-road climbs between strata).
		const Color wildStairColor = fromHex(0x8A5A24);
		for (const PlanStairway &stairway : p_plan.stairways)
		{
			if (stairway.road)
			{
				continue;
			}
			const int cx = stairway.lowCol * scale + scale / 2;
			const int cy = titleBar + stairway.lowRow * scale + scale / 2;
			canvas.fillRect(cx - 3, cy - 3, 7, 7, {250, 250, 250});
			canvas.fillRect(cx - 2, cy - 2, 5, 5, wildStairColor);
		}

		for (const auto &[portA, portB] : p_plan.boatLinks)
		{
			canvas.drawDashedLine(
				portA.col * scale + scale / 2,
				titleBar + portA.row * scale + scale / 2,
				portB.col * scale + scale / 2,
				titleBar + portB.row * scale + scale / 2,
				fromHex(0x204A8F));
		}

		// Entities.
		for (const PlanEntity &entity : p_plan.entities)
		{
			const int cx = entity.col * scale + scale / 2;
			const int cy = titleBar + entity.row * scale + scale / 2;
			switch (entity.kind)
			{
			case PlanEntityKind::Gym:
				canvas.fillStar(cx, cy, 11.0, 4.5, {10, 10, 10});
				canvas.fillStar(cx, cy, 9.0, 3.6, fromHex(0xE322D6));
				break;
			case PlanEntityKind::City:
				canvas.fillCircle(cx, cy, 6, {10, 10, 10});
				canvas.fillCircle(cx, cy, 5, fromHex(0xF7C531));
				break;
			case PlanEntityKind::PortCity:
				canvas.fillPlus(cx, cy, 8, 6, {10, 10, 10});
				canvas.fillPlus(cx, cy, 7, 4, fromHex(0x2BE3E3));
				break;
			case PlanEntityKind::RarePoi:
				canvas.fillDiamond(cx, cy, 8, {250, 250, 250});
				canvas.fillDiamond(cx, cy, 6, fromHex(0xE33030));
				break;
			case PlanEntityKind::UncommonPoi:
			case PlanEntityKind::NormalPoi:
				canvas.strokeCross(cx, cy, 5, {250, 250, 250});
				break;
			}
		}

		// Zone labels at their centroids.
		for (const PlanZone &zone : p_plan.zones)
		{
			long long sumRow = 0;
			long long sumCol = 0;
			long long count = 0;
			for (int row = 0; row < size; ++row)
			{
				for (int col = 0; col < size; ++col)
				{
					if (p_plan.zone.at(row, col) == zone.id)
					{
						sumRow += row;
						sumCol += col;
						++count;
					}
				}
			}
			if (count == 0)
			{
				continue;
			}
			const int cx = static_cast<int>(sumCol / count) * scale + scale / 2;
			const int cy = titleBar + static_cast<int>(sumRow / count) * scale;
			drawLabelBox(canvas, cx, cy, {"Z" + std::to_string(zone.id), p_plan.biomes[zone.biomeIndex].displayName}, 2);
		}

		// Legend (top-right).
		{
			const int entryHeight = 26;
			const std::vector<std::string> labels = {
				"ROAD", "BRIDGE", "WILD STAIRS", "GYM CITY", "CITY", "PORT CITY", "RARE POI", "POI"};
			int widest = 0;
			for (const std::string &label : labels)
			{
				widest = std::max(widest, textWidth(label, 2));
			}
			const int boxWidth = widest + 46;
			const int boxHeight = static_cast<int>(labels.size()) * entryHeight + 14;
			const int boxX = canvas.width - boxWidth - 10;
			const int boxY = titleBar + 10;
			canvas.fillRect(boxX, boxY, boxWidth, boxHeight, {238, 238, 230});
			for (std::size_t index = 0; index < labels.size(); ++index)
			{
				const int cx = boxX + 16;
				const int cy = boxY + 12 + static_cast<int>(index) * entryHeight + entryHeight / 2 - 6;
				switch (index)
				{
				case 0: canvas.fillRect(cx - 3, cy - 3, 6, 6, roadColor); break;
				case 1: canvas.fillRect(cx - 3, cy - 3, 6, 6, bridgeColor); break;
				case 2:
					canvas.fillRect(cx - 3, cy - 3, 7, 7, {120, 120, 120});
					canvas.fillRect(cx - 2, cy - 2, 5, 5, wildStairColor);
					break;
				case 3:
					canvas.fillStar(cx, cy, 9.0, 3.6, {10, 10, 10});
					canvas.fillStar(cx, cy, 7.5, 3.0, fromHex(0xE322D6));
					break;
				case 4:
					canvas.fillCircle(cx, cy, 6, {10, 10, 10});
					canvas.fillCircle(cx, cy, 5, fromHex(0xF7C531));
					break;
				case 5:
					canvas.fillPlus(cx, cy, 7, 5, {10, 10, 10});
					canvas.fillPlus(cx, cy, 6, 3, fromHex(0x2BE3E3));
					break;
				case 6:
					canvas.fillDiamond(cx, cy, 7, {250, 250, 250});
					canvas.fillDiamond(cx, cy, 5, fromHex(0xE33030));
					break;
				default: canvas.strokeCross(cx, cy, 4, {60, 60, 60}); break;
				}
				drawText(canvas, boxX + 34, cy - 7, labels[index], {20, 20, 20}, 2);
			}
		}

		const std::string title = "WORLD SKELETON - SEED " + std::to_string(p_plan.config.masterSeed) + " - " +
								  std::to_string(size) + "X" + std::to_string(size) + " CELLS";
		drawText(canvas, (canvas.width - textWidth(title, 2)) / 2, 10, title, {235, 240, 245}, 2);

		const std::string pathUtf8 = p_path.string();
		return stbi_write_png(pathUtf8.c_str(), canvas.width, canvas.height, 3, canvas.pixels.data(), canvas.width * 3) != 0;
	}
}
