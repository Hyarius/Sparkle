#include "structures/graphics/texture/spk_font.hpp"

#include <memory>

namespace spk
{
	Font::Text Font::textFromUTF8(std::string_view p_text)
	{
		static constexpr Codepoint ReplacementCodepoint = U'\uFFFD';

		Text result;
		result.reserve(p_text.size());

		for (std::size_t i = 0; i < p_text.size();)
		{
			const auto byte = static_cast<unsigned char>(p_text[i]);

			if (byte < 0x80)
			{
				result.push_back(static_cast<Codepoint>(byte));
				++i;
				continue;
			}

			Codepoint codepoint = 0;
			std::size_t sequenceLength = 0;
			Codepoint minimumCodepoint = 0;

			if ((byte & 0xE0) == 0xC0)
			{
				codepoint = static_cast<Codepoint>(byte & 0x1F);
				sequenceLength = 2;
				minimumCodepoint = 0x80;
			}
			else if ((byte & 0xF0) == 0xE0)
			{
				codepoint = static_cast<Codepoint>(byte & 0x0F);
				sequenceLength = 3;
				minimumCodepoint = 0x800;
			}
			else if ((byte & 0xF8) == 0xF0)
			{
				codepoint = static_cast<Codepoint>(byte & 0x07);
				sequenceLength = 4;
				minimumCodepoint = 0x10000;
			}
			else
			{
				result.push_back(ReplacementCodepoint);
				++i;
				continue;
			}

			if (i + sequenceLength > p_text.size())
			{
				result.push_back(ReplacementCodepoint);
				break;
			}

			bool validSequence = true;
			for (std::size_t offset = 1; offset < sequenceLength; ++offset)
			{
				const auto continuation = static_cast<unsigned char>(p_text[i + offset]);
				if ((continuation & 0xC0) != 0x80)
				{
					validSequence = false;
					break;
				}
				codepoint = static_cast<Codepoint>((codepoint << 6) | (continuation & 0x3F));
			}

			if (validSequence == false ||
				codepoint < minimumCodepoint ||
				codepoint > 0x10FFFF ||
				(codepoint >= 0xD800 && codepoint <= 0xDFFF))
			{
				result.push_back(ReplacementCodepoint);
				++i;
				continue;
			}

			result.push_back(codepoint);
			i += sequenceLength;
		}

		return result;
	}

	void Font::Glyph::rescale(const spk::Vector2 &p_scaleRatio)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			uvs[i] *= p_scaleRatio;
		}
	}

	Font::Atlas::Resource::Resource(
		const stbtt_fontinfo &p_fontInfo,
		size_t p_textSize,
		size_t p_outlineSize) :
		fontInfo(p_fontInfo),
		textSize(p_textSize),
		outlineSize(p_outlineSize)
	{
	}

	void Font::Atlas::_rescaleGlyphs(const spk::Vector2 &p_scaleRatio)
	{
		for (auto &[key, glyph] : _resource->glyphs)
		{
			glyph.rescale(p_scaleRatio);
		}
	}

	void Font::Atlas::_resizeData(const spk::Vector2UInt &p_size)
	{
		std::vector<uint8_t> newPixels(p_size.x * p_size.y, 0x00);

		for (size_t x = 0; x < _resource->atlasSize.x; ++x)
		{
			for (size_t y = 0; y < _resource->atlasSize.y; ++y)
			{
				newPixels[x + y * p_size.x] = _resource->atlasPixels[x + y * _resource->atlasSize.x];
			}
		}

		_rescaleGlyphs(static_cast<spk::Vector2>(_resource->atlasSize) / static_cast<spk::Vector2>(p_size));
		_resource->atlasSize = p_size;
		_resource->quadrantSize = _resource->atlasSize / 2;
		_resource->atlasPixels.swap(newPixels);
	}

	void Font::Atlas::_applyGlyphPixel(
		const uint8_t *p_pixelsToApply,
		const spk::Vector2Int &p_glyphPosition,
		const spk::Vector2UInt &p_glyphSize)
	{
		while ((p_glyphPosition.x + static_cast<int>(p_glyphSize.x) >= static_cast<int>(_resource->atlasSize.x)) ||
			   (p_glyphPosition.y + static_cast<int>(p_glyphSize.y) >= static_cast<int>(_resource->atlasSize.y)))
		{
			_resizeData(_resource->atlasSize * 2);
		}

		for (size_t x = 0; x < p_glyphSize.x; ++x)
		{
			for (size_t y = 0; y < p_glyphSize.y; ++y)
			{
				_resource->atlasPixels[(static_cast<size_t>(p_glyphPosition.x) + x) + (static_cast<size_t>(p_glyphPosition.y) + y) * _resource->atlasSize.x] =
					p_pixelsToApply[x + y * p_glyphSize.x];
			}
		}
	}

	void Font::Atlas::_uploadTexture()
	{
		setPixels(_resource->atlasPixels.data(), _resource->atlasSize, Texture::Format::GreyLevel);
	}

	Font::Atlas::Atlas(
		const stbtt_fontinfo &p_fontInfo,
		const Data &p_fontData,
		size_t p_textSize,
		size_t p_outlineSize,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap) :
		_resource(std::make_shared<Resource>(p_fontInfo, p_textSize, p_outlineSize))
	{
		(void)p_fontData;

		setProperties(p_filtering, p_wrap, p_mipmap);

		Glyph spaceGlyph;
		spaceGlyph.step = spk::Vector2Int(static_cast<int>(p_textSize / 2), 0);
		_resource->glyphs[U' '] = spaceGlyph;

		_resizeData(spk::Vector2UInt(124, 124));

		_resource->currentQuadrant = Resource::Quadrant::TopLeft;
		_resource->quadrantAnchor = spk::Vector2Int(0, 0);
		_resource->quadrantSize = _resource->atlasSize;
		_resource->nextGlyphAnchor = _resource->quadrantAnchor;
		_resource->nextLineAnchor = _resource->quadrantAnchor;
	}

	Font::Atlas::Contract Font::Atlas::subscribe(const Job &p_job)
	{
		return _resource->onEditionContractProvider.subscribe(p_job);
	}

	void Font::Atlas::loadGlyphs(const Text &p_glyphsToLoad)
	{
		for (Codepoint c : p_glyphsToLoad)
		{
			glyph(c);
		}
	}

	void Font::Atlas::loadGlyphs(std::string_view p_utf8GlyphsToLoad)
	{
		loadGlyphs(Font::textFromUTF8(p_utf8GlyphsToLoad));
	}

	const Font::Glyph &Font::Atlas::operator[](Codepoint p_codepoint)
	{
		return glyph(p_codepoint);
	}

	const Font::Glyph &Font::Atlas::glyph(Codepoint p_codepoint)
	{
		if (_resource->glyphs.contains(p_codepoint) == false)
		{
			_loadGlyph(p_codepoint);
			_uploadTexture();
			_resource->onEditionContractProvider.trigger();
		}
		return _resource->glyphs.at(p_codepoint);
	}

	spk::Vector2UInt Font::Atlas::computeCharSize(Codepoint p_codepoint)
	{
		return glyph(p_codepoint).size;
	}

	spk::Vector2UInt Font::Atlas::computeStringSize(const Text &p_string)
	{
		int totalWidth = 0;
		int maxHeight = 0;
		int minHeight = 0;

		for (Codepoint c : p_string)
		{
			const Glyph &g = glyph(c);
			totalWidth += g.step.x;
			maxHeight = std::max(maxHeight, g.positions[3].y);
			minHeight = std::min(minHeight, g.positions[0].y);
		}

		return spk::Vector2UInt(
			static_cast<unsigned int>(totalWidth),
			static_cast<unsigned int>(maxHeight - minHeight));
	}

	spk::Vector2UInt Font::Atlas::computeStringSize(std::string_view p_utf8String)
	{
		return computeStringSize(Font::textFromUTF8(p_utf8String));
	}

	spk::Vector2Int Font::Atlas::computeStringBaselineOffset(const Text &p_string)
	{
		spk::Vector2Int result(0, 0);

		for (size_t i = 0; i < p_string.size(); ++i)
		{
			const Glyph &g = glyph(p_string[i]);
			if (i == 0)
			{
				result.x = g.baselineOffset.x;
			}
			result.y = std::max(result.y, g.baselineOffset.y);
		}

		return result;
	}

	spk::Vector2Int Font::Atlas::computeStringBaselineOffset(std::string_view p_utf8String)
	{
		return computeStringBaselineOffset(Font::textFromUTF8(p_utf8String));
	}

	Font Font::fromRawData(
		const std::vector<uint8_t> &p_data,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap)
	{
		if (p_data.empty() == true)
		{
			throw std::runtime_error("Font can't be initialized from an empty data.");
		}
		Font result;
		result._loadFromData(p_data);
		result.setProperties(p_filtering, p_wrap, p_mipmap);
		return result;
	}

	Font::Font()
	{
	}

	Font::Font(const std::filesystem::path &p_path)
	{
		_loadFromFile(p_path);
	}

	void Font::setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap)
	{
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;

		for (auto &[size, atlasEntry] : _atlases)
		{
			if (atlasEntry != nullptr)
			{
				atlasEntry->setProperties(_filtering, _wrap, _mipmap);
			}
		}
	}

	spk::Vector2UInt Font::computeCharSize(Codepoint p_codepoint, const Size &p_size)
	{
		return atlas(p_size)->computeCharSize(p_codepoint);
	}

	spk::Vector2UInt Font::computeStringSize(const Text &p_string, const Size &p_size)
	{
		return atlas(p_size)->computeStringSize(p_string);
	}

	spk::Vector2UInt Font::computeStringSize(std::string_view p_utf8String, const Size &p_size)
	{
		return computeStringSize(textFromUTF8(p_utf8String), p_size);
	}

	spk::Vector2Int Font::computeStringBaselineOffset(const Text &p_string, const Size &p_size)
	{
		return atlas(p_size)->computeStringBaselineOffset(p_string);
	}

	spk::Vector2Int Font::computeStringBaselineOffset(std::string_view p_utf8String, const Size &p_size)
	{
		return computeStringBaselineOffset(textFromUTF8(p_utf8String), p_size);
	}

	Font::Size Font::computeOptimalTextSize(
		const Text &p_string,
		float p_outlineSizeRatio,
		const spk::Vector2UInt &p_textArea)
	{
		static const std::vector<size_t> deltas = {100u, 50u, 20u, 10u, 1u};
		Size result(2, 0);

		if (p_string.empty())
		{
			const size_t outlineSize = static_cast<size_t>(p_textArea.y * p_outlineSizeRatio);
			return Size(p_textArea.y - outlineSize * 2, outlineSize);
		}

		for (size_t delta : deltas)
		{
			if (delta > p_textArea.y)
			{
				continue;
			}

			bool enough = false;
			while (enough == false)
			{
				const size_t glyphSize = result.glyph + delta;
				const size_t outlineSize = static_cast<size_t>(glyphSize * p_outlineSizeRatio);
				const spk::Vector2UInt tmpSize = computeStringSize(p_string, {glyphSize - outlineSize * 2, outlineSize});

				if (tmpSize.x >= p_textArea.x || tmpSize.y >= p_textArea.y)
				{
					enough = true;
				}
				else
				{
					result.glyph += delta;
				}
			}
		}

		result.outline = static_cast<size_t>(result.glyph * p_outlineSizeRatio);
		result.glyph -= result.outline * 2;

		return result;
	}

	Font::Size Font::computeOptimalTextSize(
		std::string_view p_utf8String,
		float p_outlineSizeRatio,
		const spk::Vector2UInt &p_textArea)
	{
		return computeOptimalTextSize(textFromUTF8(p_utf8String), p_outlineSizeRatio, p_textArea);
	}

	std::shared_ptr<Font::Atlas> Font::atlas(const Size &p_size)
	{
		auto it = _atlases.find(p_size);
		if (it == _atlases.end() || it->second == nullptr)
		{
			auto atlasEntry = std::make_shared<Atlas>(
				_fontInfo,
				_fontData,
				p_size.glyph,
				p_size.outline,
				_filtering,
				_wrap,
				_mipmap);
			it = _atlases.insert_or_assign(p_size, std::move(atlasEntry)).first;
		}
		return it->second;
	}
}
