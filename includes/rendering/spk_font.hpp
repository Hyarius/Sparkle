#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "stb_truetype.h"

#include "opengl/spk_opengl_texture.hpp"
#include "rendering/spk_texture.hpp"
#include "utils/spk_contract_provider.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	class Font
	{
	public:
		using Data = std::vector<uint8_t>;
		using Filtering = Texture::Filtering;
		using Wrap = Texture::Wrap;
		using Mipmap = Texture::Mipmap;

		struct Glyph
		{
			std::array<spk::Vector2Int, 4> positions;
			std::array<spk::Vector2, 4> uvs;
			spk::Vector2Int step;
			spk::Vector2Int baselineOffset;
			spk::Vector2UInt size;

			static inline std::vector<unsigned int> indexesOrder = {0, 1, 2, 2, 1, 3};

			void rescale(const spk::Vector2& p_scaleRatio);
		};

		struct Size
		{
			size_t glyph = 0;
			size_t outline = 0;

			constexpr Size() = default;

			constexpr Size(size_t p_glyph) :
				glyph(p_glyph),
				outline(0)
			{
			}

			constexpr Size(size_t p_glyph, size_t p_outline) :
				glyph(p_glyph),
				outline(p_outline)
			{
			}

			constexpr bool operator==(const Size& p_other) const
			{
				return glyph == p_other.glyph && outline == p_other.outline;
			}

			constexpr bool operator!=(const Size& p_other) const
			{
				return (*this == p_other) == false;
			}

			constexpr bool operator<(const Size& p_other) const
			{
				return glyph < p_other.glyph || (glyph == p_other.glyph && outline < p_other.outline);
			}
		};

		class Atlas : public spk::OpenGL::Texture
		{
			friend class Font;

		public:
			using Contract = spk::ContractProvider<>::Contract;
			using Job = spk::ContractProvider<>::Callback;

		private:
			spk::ContractProvider<> _onEditionContractProvider;
			const stbtt_fontinfo& _fontInfo;

			std::unordered_map<wchar_t, Glyph> _glyphs;
			Glyph _unknownGlyph;

			std::vector<uint8_t> _atlasPixels;

			enum class Quadrant
			{
				TopLeft,
				TopRight,
				DownLeft,
				DownRight
			};

			Quadrant _currentQuadrant = Quadrant::TopLeft;
			spk::Vector2Int _quadrantAnchor{0, 0};
			spk::Vector2UInt _quadrantSize{0, 0};
			spk::Vector2Int _nextGlyphAnchor{0, 0};
			spk::Vector2Int _nextLineAnchor{0, 0};
			spk::Vector2UInt _atlasSize{0, 0};

			size_t _textSize;
			size_t _outlineSize;

			void _rescaleGlyphs(const spk::Vector2& p_scaleRatio);
			void _resizeData(const spk::Vector2UInt& p_size);
			spk::Vector2Int _computeGlyphPosition(const spk::Vector2UInt& p_glyphSize);
			void _applyGlyphPixel(
				const uint8_t* p_pixelsToApply,
				const spk::Vector2Int& p_glyphPosition,
				const spk::Vector2UInt& p_glyphSize);
			void _loadGlyph(wchar_t p_char);
			void _uploadTexture();

		public:
			Atlas(
				const stbtt_fontinfo& p_fontInfo,
				const Data& p_fontData,
				size_t p_textSize,
				size_t p_outlineSize,
				Filtering p_filtering = Filtering::Linear,
				Wrap p_wrap = Wrap::ClampToEdge,
				Mipmap p_mipmap = Mipmap::Enable);

			Contract subscribe(const Job& p_job);

			void loadGlyphs(const std::wstring& p_glyphsToLoad);
			void loadAllRenderableGlyphs();

			const Glyph& operator[](wchar_t p_char);
			const Glyph& glyph(wchar_t p_char);

			spk::Vector2UInt computeCharSize(wchar_t p_char);
			spk::Vector2UInt computeStringSize(const std::wstring& p_string);
			spk::Vector2Int computeStringBaselineOffset(const std::wstring& p_string);
		};

	private:
		void _loadFromFile(const std::filesystem::path& p_path);
		void _loadFromData(const std::vector<uint8_t>& p_data);

		std::map<Size, Atlas> _atlases;
		Data _fontData;
		stbtt_fontinfo _fontInfo;

		Filtering _filtering = Filtering::Nearest;
		Wrap _wrap = Wrap::ClampToEdge;
		Mipmap _mipmap = Mipmap::Disable;

	public:
		static Font fromRawData(
			const std::vector<uint8_t>& p_data,
			Filtering p_filtering = Filtering::Nearest,
			Wrap p_wrap = Wrap::ClampToEdge,
			Mipmap p_mipmap = Mipmap::Disable);

		Font();
		explicit Font(const std::filesystem::path& p_path);

		void setProperties(Filtering p_filtering, Wrap p_wrap, Mipmap p_mipmap);

		spk::Vector2UInt computeCharSize(wchar_t p_char, const Size& p_size);
		spk::Vector2UInt computeStringSize(const std::wstring& p_string, const Size& p_size);
		spk::Vector2Int computeStringBaselineOffset(const std::wstring& p_string, const Size& p_size);

		Size computeOptimalTextSize(
			const std::wstring& p_string,
			float p_outlineSizeRatio,
			const spk::Vector2UInt& p_textArea);

		Atlas& atlas(const Size& p_size);
	};
}

#endif
