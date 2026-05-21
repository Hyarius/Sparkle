#include "rendering/render_command/spk_text_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <utility>

namespace spk
{
	TextRenderCommand::TextRenderCommand(
		const spk::Font& p_font,
		std::wstring p_text,
		spk::Font::Size p_size,
		spk::Color p_color,
		float p_depth,
		spk::Vector2Int p_anchor,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment) :
		_font(p_font),
		_text(std::move(p_text)),
		_size(p_size),
		_color(p_color),
		_depth(p_depth),
		_anchor(p_anchor),
		_horizontalAlignment(p_horizontalAlignment),
		_verticalAlignment(p_verticalAlignment)
	{
	}

	spk::Vector2Int TextRenderCommand::_computeBaselinePosition() const
	{
		const spk::Vector2UInt stringSize = const_cast<spk::Font&>(_font).computeStringSize(_text, _size);


		int baselineX = _anchor.x;
		switch (_horizontalAlignment)
		{
		case spk::HorizontalAlignment::Left:
			baselineX = _anchor.x;
			break;
		case spk::HorizontalAlignment::Centered:
			baselineX = _anchor.x - static_cast<int>(stringSize.x) / 2;
			break;
		case spk::HorizontalAlignment::Right:
			baselineX = _anchor.x - static_cast<int>(stringSize.x);
			break;
		}

		int baselineY = _anchor.y;
		switch (_verticalAlignment)
		{
		case spk::VerticalAlignment::Top:
			baselineY = _anchor.y;
			break;
		case spk::VerticalAlignment::Centered:
			baselineY = _anchor.y - static_cast<int>(stringSize.y) / 2;
			break;
		case spk::VerticalAlignment::Down:
			baselineY = _anchor.y - static_cast<int>(stringSize.y);
			break;
		}

		return {baselineX, baselineY};
	}

	void TextRenderCommand::_ensureFontCommand() const
	{
		if (_fontCommand == nullptr)
		{
			_fontCommand = std::make_unique<spk::DrawFontRenderCommand>(
				_font,
				_text,
				_computeBaselinePosition(),
				_size,
				_color,
				_depth);
		}
	}

	void TextRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		_ensureFontCommand();
		_fontCommand->execute(p_renderContext);
	}
}

#endif
