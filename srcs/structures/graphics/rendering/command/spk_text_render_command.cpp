#include "structures/graphics/rendering/command/spk_text_render_command.hpp"

#include <utility>

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
		spk::VerticalAlignment p_verticalAlignment)
	{
		const spk::Vector2UInt stringSize = p_font.computeStringSize(p_text, p_size);
		const spk::Vector2Int baselineOffset = p_font.computeStringBaselineOffset(p_text, p_size);

		int baselineX = p_anchor.x;
		switch (p_horizontalAlignment)
		{
		case spk::HorizontalAlignment::Left:
			baselineX = p_anchor.x + baselineOffset.x;
			break;
		case spk::HorizontalAlignment::Centered:
			baselineX = p_anchor.x - static_cast<int>(stringSize.x) / 2 + baselineOffset.x;
			break;
		case spk::HorizontalAlignment::Right:
			baselineX = p_anchor.x - static_cast<int>(stringSize.x) + baselineOffset.x;
			break;
		}

		int baselineY = p_anchor.y;
		switch (p_verticalAlignment)
		{
		case spk::VerticalAlignment::Top:
			baselineY = p_anchor.y + baselineOffset.y;
			break;
		case spk::VerticalAlignment::Centered:
			baselineY = p_anchor.y - static_cast<int>(stringSize.y) / 2 + baselineOffset.y;
			break;
		case spk::VerticalAlignment::Down:
			baselineY = p_anchor.y - static_cast<int>(stringSize.y) + baselineOffset.y;
			break;
		}

		_fontCommand = std::make_unique<spk::DrawFontRenderCommand>(
			p_font,
			std::move(p_text),
			spk::Vector2Int{baselineX, baselineY},
			p_size,
			p_glyphColor,
			p_outlineColor,
			p_depth);
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
		_fontCommand->execute(p_renderContext);
	}
}
