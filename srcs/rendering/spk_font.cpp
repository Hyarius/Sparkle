#include "rendering/spk_font.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk
{
	void Font::Glyph::rescale(const spk::Vector2& p_scaleRatio)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			uvs[i] *= p_scaleRatio;
		}
	}

	void Font::Atlas::_rescaleGlyphs(const spk::Vector2& p_scaleRatio)
	{
		for (auto& [key, glyph] : _glyphs)
		{
			glyph.rescale(p_scaleRatio);
		}
	}

	void Font::Atlas::_resizeData(const spk::Vector2UInt& p_size)
	{
		std::vector<uint8_t> newPixels(p_size.x * p_size.y, 0x00);

		for (size_t x = 0; x < _atlasSize.x; ++x)
		{
			for (size_t y = 0; y < _atlasSize.y; ++y)
			{
				newPixels[x + y * p_size.x] = _atlasPixels[x + y * _atlasSize.x];
			}
		}

		_rescaleGlyphs(static_cast<spk::Vector2>(_atlasSize) / static_cast<spk::Vector2>(p_size));
		_atlasSize = p_size;
		_quadrantSize = _atlasSize / 2;
		_atlasPixels.swap(newPixels);
	}

	void Font::Atlas::_applyGlyphPixel(
		const uint8_t* p_pixelsToApply,
		const spk::Vector2Int& p_glyphPosition,
		const spk::Vector2UInt& p_glyphSize)
	{
		while ((p_glyphPosition.x + static_cast<int>(p_glyphSize.x) >= static_cast<int>(_atlasSize.x)) ||
			   (p_glyphPosition.y + static_cast<int>(p_glyphSize.y) >= static_cast<int>(_atlasSize.y)))
		{
			_resizeData(_atlasSize * 2);
		}

		for (size_t x = 0; x < p_glyphSize.x; ++x)
		{
			for (size_t y = 0; y < p_glyphSize.y; ++y)
			{
				_atlasPixels[
					(static_cast<size_t>(p_glyphPosition.x) + x) +
					(static_cast<size_t>(p_glyphPosition.y) + y) * _atlasSize.x] =
					p_pixelsToApply[x + y * p_glyphSize.x];
			}
		}
	}

	void Font::Atlas::_uploadTexture()
	{
		setPixels(_atlasPixels.data(), _atlasSize, Texture::Format::GreyLevel);
	}

	Font::Atlas::Atlas(
		const stbtt_fontinfo& p_fontInfo,
		const Data& p_fontData,
		size_t p_textSize,
		size_t p_outlineSize,
		Filtering p_filtering,
		Wrap p_wrap,
		Mipmap p_mipmap) :
		_fontInfo(p_fontInfo),
		_textSize(p_textSize),
		_outlineSize(p_outlineSize)
	{
		(void)p_fontData;

		setProperties(p_filtering, p_wrap, p_mipmap);

		Glyph spaceGlyph;
		spaceGlyph.step = spk::Vector2Int(static_cast<int>(p_textSize / 2), 0);
		_glyphs[L' '] = spaceGlyph;

		_resizeData(spk::Vector2UInt(124, 124));

		_currentQuadrant = Quadrant::TopLeft;
		_quadrantAnchor = spk::Vector2Int(0, 0);
		_quadrantSize = _atlasSize;
		_nextGlyphAnchor = _quadrantAnchor;
		_nextLineAnchor = _quadrantAnchor;
	}

	Font::Atlas::Contract Font::Atlas::subscribe(const Job& p_job)
	{
		return _onEditionContractProvider.subscribe(p_job);
	}

	void Font::Atlas::loadGlyphs(const std::wstring& p_glyphsToLoad)
	{
		for (wchar_t c : p_glyphsToLoad)
		{
			glyph(c);
		}
	}

	const Font::Glyph& Font::Atlas::operator[](wchar_t p_char)
	{
		return glyph(p_char);
	}

	const Font::Glyph& Font::Atlas::glyph(wchar_t p_char)
	{
		if (_glyphs.contains(p_char) == false)
		{
			_loadGlyph(p_char);
			_uploadTexture();
			_onEditionContractProvider.trigger();
		}
		return _glyphs.at(p_char);
	}

	spk::Vector2UInt Font::Atlas::computeCharSize(wchar_t p_char)
	{
		return glyph(p_char).size;
	}

	spk::Vector2UInt Font::Atlas::computeStringSize(const std::wstring& p_string)
	{
		int totalWidth = 0;
		int maxHeight = 0;
		int minHeight = 0;

		for (wchar_t c : p_string)
		{
			const Glyph& g = glyph(c);
			totalWidth += g.step.x;
			maxHeight = std::max(maxHeight, g.positions[3].y);
			minHeight = std::min(minHeight, g.positions[0].y);
		}

		return spk::Vector2UInt(
			static_cast<unsigned int>(totalWidth),
			static_cast<unsigned int>(maxHeight - minHeight));
	}

	spk::Vector2Int Font::Atlas::computeStringBaselineOffset(const std::wstring& p_string)
	{
		spk::Vector2Int result(0, 0);

		for (size_t i = 0; i < p_string.size(); ++i)
		{
			const Glyph& g = glyph(p_string[i]);
			if (i == 0)
			{
				result.x = g.baselineOffset.x;
			}
			result.y = std::max(result.y, g.baselineOffset.y);
		}

		return result;
	}

	Font Font::fromRawData(
		const std::vector<uint8_t>& p_data,
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

	Font::Font(const std::filesystem::path& p_path)
	{
		_loadFromFile(p_path);
	}

	void Font::setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap)
	{
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;

		for (auto& [size, atlasEntry] : _atlases)
		{
			atlasEntry.setProperties(_filtering, _wrap, _mipmap);
		}
	}

	spk::Vector2UInt Font::computeCharSize(wchar_t p_char, const Size& p_size)
	{
		return atlas(p_size).computeCharSize(p_char);
	}

	spk::Vector2UInt Font::computeStringSize(const std::wstring& p_string, const Size& p_size)
	{
		return atlas(p_size).computeStringSize(p_string);
	}

	spk::Vector2Int Font::computeStringBaselineOffset(const std::wstring& p_string, const Size& p_size)
	{
		return atlas(p_size).computeStringBaselineOffset(p_string);
	}

	Font::Size Font::computeOptimalTextSize(
		const std::wstring& p_string,
		float p_outlineSizeRatio,
		const spk::Vector2UInt& p_textArea)
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

	Font::Atlas& Font::atlas(const Size& p_size)
	{
		if (_atlases.find(p_size) == _atlases.end())
		{
			_atlases.try_emplace(p_size, _fontInfo, _fontData, p_size.glyph, p_size.outline, _filtering, _wrap, _mipmap);
		}
		return _atlases.at(p_size);
	}
}

#endif
