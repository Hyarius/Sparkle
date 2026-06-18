#include "structures/graphics/rendering/command/spk_text_render_command.hpp"

#include <array>
#include <memory>
#include <utility>

namespace
{
	[[nodiscard]] spk::Vector3 toPosition(const spk::Vector2Int& p_pixel, float p_depth)
	{
		return {static_cast<float>(p_pixel.x), static_cast<float>(p_pixel.y), p_depth};
	}

	[[nodiscard]] std::shared_ptr<spk::TextureMesh2D> buildTextMesh(
		spk::Font::Atlas& p_atlas, const spk::Font::Text& p_text, const spk::Vector2Int& p_baselinePosition, float p_depth)
	{
		p_atlas.loadGlyphs(p_text);

		auto mesh = std::make_shared<spk::TextureMesh2D>();
		int cursorX = p_baselinePosition.x;

		for (spk::Font::Codepoint character : p_text)
		{
			const spk::Font::Glyph& glyph = p_atlas.glyph(character);

			if (glyph.size.x != 0 && glyph.size.y != 0)
			{
				std::array<spk::TextureVertex2D, 4> vertices;

				for (std::size_t i = 0; i < vertices.size(); ++i)
				{
					const spk::Vector2Int pixelPosition = {cursorX + glyph.positions[i].x, p_baselinePosition.y + glyph.positions[i].y};

					vertices[i] = {toPosition(pixelPosition, p_depth), glyph.uvs[i]};
				}

				mesh->addShape(vertices[0], vertices[1], vertices[3], vertices[2]);
			}

			cursorX += glyph.step.x;
		}

		return mesh;
	}
}

namespace spk
{
	TextRenderCommand::TextRenderCommand(
		spk::Font& p_font,
		spk::Font::Text p_text,
		spk::Font::Size p_size,
		spk::Color p_glyphColor,
		spk::Color p_outlineColor,
		float p_depth,
		spk::Vector2Int p_anchor,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment) :
		_font(p_font),
		_atlas(p_font.atlas(p_size)),
		_text(std::move(p_text)),
		_size(p_size),
		_glyphColor(p_glyphColor),
		_outlineColor(p_outlineColor),
		_depth(p_depth),
		_anchor(p_anchor),
		_horizontalAlignment(p_horizontalAlignment),
		_verticalAlignment(p_verticalAlignment)
	{
		_rebuildFontCommand();
		_onAtlasEditionContract = _atlas.subscribe([this]() { _fontCommandOutdated = true; });
	}

	void TextRenderCommand::_rebuildFontCommand()
	{
		const spk::Vector2UInt stringSize = _font.computeStringSize(_text, _size);
		const spk::Vector2Int baselineOffset = _font.computeStringBaselineOffset(_text, _size);

		int baselineX = _anchor.x;
		switch (_horizontalAlignment)
		{
		case spk::HorizontalAlignment::Left:
			baselineX = _anchor.x + baselineOffset.x;
			break;
		case spk::HorizontalAlignment::Centered:
			baselineX = _anchor.x - static_cast<int>(stringSize.x) / 2 + baselineOffset.x;
			break;
		case spk::HorizontalAlignment::Right:
			baselineX = _anchor.x - static_cast<int>(stringSize.x) + baselineOffset.x;
			break;
		}

		int baselineY = _anchor.y;
		switch (_verticalAlignment)
		{
		case spk::VerticalAlignment::Top:
			baselineY = _anchor.y + baselineOffset.y;
			break;
		case spk::VerticalAlignment::Centered:
			baselineY = _anchor.y - static_cast<int>(stringSize.y) / 2 + baselineOffset.y;
			break;
		case spk::VerticalAlignment::Down:
			baselineY = _anchor.y - static_cast<int>(stringSize.y) + baselineOffset.y;
			break;
		}

		_fontCommand = std::make_unique<spk::DrawFontRenderCommand>(
			_atlas, buildTextMesh(_atlas, _text, spk::Vector2Int{baselineX, baselineY}, _depth), _size, _glyphColor, _outlineColor);

		_fontCommandOutdated = false;
	}

	TextRenderCommand::TextRenderCommand(
		spk::Font& p_font,
		std::string_view p_text,
		spk::Font::Size p_size,
		spk::Color p_glyphColor,
		spk::Color p_outlineColor,
		float p_depth,
		spk::Vector2Int p_anchor,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment) :
		TextRenderCommand(
			p_font,
			spk::Font::textFromUTF8(p_text),
			p_size,
			p_glyphColor,
			p_outlineColor,
			p_depth,
			p_anchor,
			p_horizontalAlignment,
			p_verticalAlignment)
	{
	}

	void TextRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		if (_fontCommandOutdated == true || _fontCommand == nullptr)
		{
			_rebuildFontCommand();
		}

		_fontCommand->execute(p_renderContext);
	}
}
