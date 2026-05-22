#include "rendering/render_command/spk_text_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <utility>

namespace spk
{
	TextRenderCommand::TextRenderCommand(
		const spk::Font& p_font,
		std::wstring p_text,
		spk::Font::Size p_size,
		spk::Color p_glyphColor,
		spk::Color p_outlineColor,
		float p_depth,
		spk::Vector2Int p_anchor,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment)
	{
		const spk::Vector2UInt stringSize = const_cast<spk::Font&>(p_font).computeStringSize(p_text, p_size);

		int baselineX = p_anchor.x;
		switch (p_horizontalAlignment)
		{
		case spk::HorizontalAlignment::Left:
			baselineX = p_anchor.x;
			break;
		case spk::HorizontalAlignment::Centered:
			baselineX = p_anchor.x - static_cast<int>(stringSize.x) / 2;
			break;
		case spk::HorizontalAlignment::Right:
			baselineX = p_anchor.x - static_cast<int>(stringSize.x);
			break;
		}

		int baselineY = p_anchor.y;
		switch (p_verticalAlignment)
		{
		case spk::VerticalAlignment::Top:
			baselineY = p_anchor.y;
			break;
		case spk::VerticalAlignment::Centered:
			baselineY = p_anchor.y - static_cast<int>(stringSize.y) / 2;
			break;
		case spk::VerticalAlignment::Down:
			baselineY = p_anchor.y - static_cast<int>(stringSize.y);
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

	void TextRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		_fontCommand->execute(p_renderContext);
	}
}

#endif
