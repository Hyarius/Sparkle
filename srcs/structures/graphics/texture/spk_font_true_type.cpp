#include "structures/graphics/texture/spk_font.hpp"

#include <fstream>
#include <unordered_set>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace spk
{
	static const std::vector<std::pair<int, int>> unicodeBlocks = {
		{0x0000, 0x007F}, // Basic Latin
		{0x0080, 0x00FF}, // Latin-1 Supplement
	};

	void Font::Atlas::loadAllRenderableGlyphs()
	{
		auto &resource = *_resource;
		std::unordered_set<int> rendered;

		for (const auto &block : unicodeBlocks)
		{
			for (int codepoint = block.first; codepoint <= block.second; ++codepoint)
			{
				if (rendered.contains(codepoint) == false)
				{
					const int glyphIndex = stbtt_FindGlyphIndex(&resource.fontInfo, codepoint);
					if (glyphIndex != 0 && resource.glyphs.contains(static_cast<Codepoint>(codepoint)) == false)
					{
						_loadGlyph(static_cast<Codepoint>(codepoint));
						rendered.insert(codepoint);
					}
				}
			}
		}

		_uploadTexture();
	}

	spk::Vector2Int Font::Atlas::_computeGlyphPosition(const spk::Vector2UInt &p_glyphSize)
	{
		auto &resource = *_resource;
		using Quadrant = Resource::Quadrant;

		if ((resource.nextGlyphAnchor.x + static_cast<int>(p_glyphSize.x)) >=
			(resource.quadrantAnchor.x + static_cast<int>(resource.quadrantSize.x)))
		{
			resource.nextGlyphAnchor = resource.nextLineAnchor;
		}

		spk::Vector2Int result = resource.nextGlyphAnchor;
		resource.nextGlyphAnchor.x += static_cast<int>(p_glyphSize.x);

		if (resource.nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
		{
			resource.nextLineAnchor.y = result.y + static_cast<int>(p_glyphSize.y);
		}

		auto _overflowQuadrant = [&]() {
			return resource.nextLineAnchor.y >= resource.quadrantAnchor.y + static_cast<int>(resource.quadrantSize.y);
		};

		auto _resetToQuadrant = [&](spk::Vector2Int p_anchor) {
			resource.quadrantAnchor = p_anchor;
			result = resource.nextGlyphAnchor = resource.nextLineAnchor = p_anchor;
			resource.nextGlyphAnchor.x += static_cast<int>(p_glyphSize.x);
			if (resource.nextLineAnchor.y < result.y + static_cast<int>(p_glyphSize.y))
			{
				resource.nextLineAnchor.y = result.y + static_cast<int>(p_glyphSize.y);
			}
		};

		switch (resource.currentQuadrant)
		{
		case Quadrant::TopLeft:
			if (_overflowQuadrant())
			{
				resource.currentQuadrant = Quadrant::TopRight;
				_resizeData(resource.atlasSize * 2);
				_resetToQuadrant(spk::Vector2Int(static_cast<int>(resource.atlasSize.x / 2), 0));
			}
			break;

		case Quadrant::TopRight:
			if (_overflowQuadrant())
			{
				resource.currentQuadrant = Quadrant::DownLeft;
				_resetToQuadrant(spk::Vector2Int(0, static_cast<int>(resource.atlasSize.y / 2)));
			}
			break;

		case Quadrant::DownLeft:
			if (_overflowQuadrant())
			{
				resource.currentQuadrant = Quadrant::DownRight;
				_resetToQuadrant(spk::Vector2Int(static_cast<int>(resource.atlasSize.x / 2), static_cast<int>(resource.atlasSize.y / 2)));
			}
			break;

		case Quadrant::DownRight:
			if (_overflowQuadrant())
			{
				resource.currentQuadrant = Quadrant::TopRight;
				_resizeData(resource.atlasSize * 2);
				_resetToQuadrant(spk::Vector2Int(static_cast<int>(resource.atlasSize.x / 2), 0));
			}
			break;
		}

		return result;
	}

	void Font::Atlas::_loadGlyph(Codepoint p_codepoint)
	{
		auto &resource = *_resource;
		const float scale = stbtt_ScaleForMappingEmToPixels(&resource.fontInfo, static_cast<float>(resource.textSize));
		const int stbCodepoint = static_cast<int>(p_codepoint);
		const size_t sdfPadding = resource.outlineSize == 0 ? 0 : resource.outlineSize + 2;

		int width = 0;
		int height = 0;
		int xOffset = 0;
		int yOffset = 0;

		uint8_t *glyphBitmap = stbtt_GetCodepointSDF(
			&resource.fontInfo,
			scale,
			stbCodepoint,
			static_cast<int>(sdfPadding),
			128,
			128.0f / static_cast<float>(sdfPadding == 0 ? 1 : sdfPadding),
			&width,
			&height,
			&xOffset,
			&yOffset);

		if (glyphBitmap == nullptr)
		{
			resource.glyphs[p_codepoint] = resource.unknownGlyph;
			return;
		}

		Glyph glyph;
		glyph.size = spk::Vector2UInt(static_cast<unsigned int>(width), static_cast<unsigned int>(height));

		const spk::Vector2Int glyphPosition = _computeGlyphPosition(glyph.size);
		_applyGlyphPixel(glyphBitmap, glyphPosition, glyph.size);

		glyph.baselineOffset = spk::Vector2Int(-xOffset, -yOffset);

		const spk::Vector2 halfPixel = 0.5f / spk::Vector2(
												  static_cast<float>(resource.atlasSize.x),
												  static_cast<float>(resource.atlasSize.y));

		glyph.positions[0] = spk::Vector2Int(xOffset, yOffset);
		glyph.positions[1] = spk::Vector2Int(xOffset, yOffset + height);
		glyph.positions[2] = spk::Vector2Int(xOffset + width, yOffset);
		glyph.positions[3] = spk::Vector2Int(xOffset + width, yOffset + height);

		glyph.uvs[0] = spk::Vector2(
			static_cast<float>(glyphPosition.x) / static_cast<float>(resource.atlasSize.x) + halfPixel.x,
			static_cast<float>(glyphPosition.y) / static_cast<float>(resource.atlasSize.y) + halfPixel.y);
		glyph.uvs[1] = spk::Vector2(
			static_cast<float>(glyphPosition.x) / static_cast<float>(resource.atlasSize.x) + halfPixel.x,
			static_cast<float>(glyphPosition.y + height) / static_cast<float>(resource.atlasSize.y) - halfPixel.y);
		glyph.uvs[2] = spk::Vector2(
			static_cast<float>(glyphPosition.x + width) / static_cast<float>(resource.atlasSize.x) - halfPixel.x,
			static_cast<float>(glyphPosition.y) / static_cast<float>(resource.atlasSize.y) + halfPixel.y);
		glyph.uvs[3] = spk::Vector2(
			static_cast<float>(glyphPosition.x + width) / static_cast<float>(resource.atlasSize.x) - halfPixel.x,
			static_cast<float>(glyphPosition.y + height) / static_cast<float>(resource.atlasSize.y) - halfPixel.y);

		int advance = 0;
		stbtt_GetCodepointHMetrics(&resource.fontInfo, stbCodepoint, &advance, nullptr);
		glyph.step = spk::Vector2Int(
			static_cast<int>(std::ceil(advance * scale)) + static_cast<int>(resource.outlineSize),
			0);

		resource.glyphs[p_codepoint] = glyph;
		stbtt_FreeBitmap(glyphBitmap, nullptr);
	}

	void Font::_loadFromData(const std::vector<uint8_t> &p_data)
	{
		_fontData = p_data;
		stbtt_InitFont(&_fontInfo, reinterpret_cast<const unsigned char *>(_fontData.data()), 0);
	}

	void Font::_loadFromFile(const std::filesystem::path &p_path)
	{
		std::ifstream file(p_path, std::ios::binary);
		if (file.is_open() == false)
		{
			throw std::runtime_error("Font: failed to open file: " + p_path.string());
		}
		_loadFromData(std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
	}
}
