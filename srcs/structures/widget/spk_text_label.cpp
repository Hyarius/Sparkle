#include "structures/widget/spk_text_label.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/command/spk_text_render_command.hpp"

namespace
{
	[[nodiscard]] bool sameColor(const spk::Color& p_left, const spk::Color& p_right)
	{
		return p_left.r == p_right.r && p_left.g == p_right.g && p_left.b == p_right.b && p_left.a == p_right.a;
	}
}

namespace spk
{
	TextLabel::TextLabel(const std::string& p_name, spk::Widget* p_parent) :
		TextLabel(p_name, "", p_parent)
	{
	}

	TextLabel::TextLabel(const std::string& p_name, std::string_view p_text, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_text(spk::Font::textFromUTF8(p_text))
	{
		_configureSizeHint();
		useDefaultStyle();
		activate();
	}

	TextLabel::TextLabel(const std::string& p_name, std::string_view p_text, const spk::WidgetStyle& p_style, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_text(spk::Font::textFromUTF8(p_text))
	{
		_configureSizeHint();
		useStyle(p_style);
		activate();
	}

	void TextLabel::_bindStyle(const spk::WidgetStyle& p_style)
	{
		_styleEditionContract = p_style.subscribeToEdition([this](const spk::WidgetStyle& p_editedStyle) { applyStyle(p_editedStyle); });
	}

	void TextLabel::applyStyle(const spk::WidgetStyle& p_style)
	{
		setFont(p_style.font());
		setTextSize(p_style.textSize());
		setGlyphColor(p_style.glyphColor());
		setOutlineColor(p_style.outlineColor());
		setPadding(p_style.textPadding());
	}

	void TextLabel::useDefaultStyle()
	{
		_bindStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
		applyStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
	}

	void TextLabel::useStyle(const spk::WidgetStyle& p_style)
	{
		_bindStyle(p_style);
		applyStyle(p_style);
	}

	void TextLabel::_configureSizeHint()
	{
		sizeHint().configureMinimalGenerator(
			[this]()
			{
				spk::Vector2UInt result = {0, 0};

				if (_font != nullptr && _text.empty() == false)
				{
					const spk::Vector2UInt textSize = _font->computeStringSize(_text, _textSize);
					result.x = textSize.x + static_cast<unsigned int>(_padding.x * 2);
					result.y = textSize.y + static_cast<unsigned int>(_padding.y * 2);
				}

				return result;
			});
	}

	spk::Vector2Int TextLabel::_textAnchor() const
	{
		const spk::Rect2D& rect = geometry();

		int x = rect.left() + _padding.x;
		if (_horizontalAlignment == spk::HorizontalAlignment::Centered)
		{
			x = rect.left() + static_cast<int>(rect.width() / 2);
		}
		else if (_horizontalAlignment == spk::HorizontalAlignment::Right)
		{
			x = rect.right() - _padding.x;
		}

		int y = rect.top() + _padding.y;
		if (_verticalAlignment == spk::VerticalAlignment::Centered)
		{
			y = rect.top() + static_cast<int>(rect.height() / 2);
		}
		else if (_verticalAlignment == spk::VerticalAlignment::Down)
		{
			y = rect.bottom() - _padding.y;
		}

		return {x, y};
	}

	spk::RenderUnit TextLabel::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (_font != nullptr && _text.empty() == false && geometry().empty() == false)
		{
			builder.emplace<spk::TextRenderCommand>(
				*_font, _text, _textSize, _glyphColor, _outlineColor, _depth, _textAnchor(), _horizontalAlignment, _verticalAlignment);
		}

		return builder.build();
	}

	void TextLabel::setFont(std::shared_ptr<spk::Font> p_font)
	{
		if (p_font == nullptr)
		{
			throw std::invalid_argument("TextLabel font cannot be null");
		}

		_font = std::move(p_font);
		invalidateRenderUnit();
	}

	void TextLabel::setText(const spk::Font::Text& p_text)
	{
		if (_text == p_text)
		{
			return;
		}

		_text = p_text;
		invalidateRenderUnit();
	}

	void TextLabel::setText(std::string_view p_text)
	{
		setText(spk::Font::textFromUTF8(p_text));
	}

	void TextLabel::setTextSize(const spk::Font::Size& p_textSize)
	{
		if (_textSize == p_textSize)
		{
			return;
		}

		_textSize = p_textSize;
		invalidateRenderUnit();
	}

	void TextLabel::setGlyphColor(const spk::Color& p_color)
	{
		if (sameColor(_glyphColor, p_color) == true)
		{
			return;
		}

		_glyphColor = p_color;
		invalidateRenderUnit();
	}

	void TextLabel::setOutlineColor(const spk::Color& p_color)
	{
		if (sameColor(_outlineColor, p_color) == true)
		{
			return;
		}

		_outlineColor = p_color;
		invalidateRenderUnit();
	}

	void TextLabel::setDepth(float p_depth)
	{
		if (_depth == p_depth)
		{
			return;
		}

		_depth = p_depth;
		invalidateRenderUnit();
	}

	void TextLabel::setHorizontalAlignment(spk::HorizontalAlignment p_alignment)
	{
		if (_horizontalAlignment == p_alignment)
		{
			return;
		}

		_horizontalAlignment = p_alignment;
		invalidateRenderUnit();
	}

	void TextLabel::setVerticalAlignment(spk::VerticalAlignment p_alignment)
	{
		if (_verticalAlignment == p_alignment)
		{
			return;
		}

		_verticalAlignment = p_alignment;
		invalidateRenderUnit();
	}

	void TextLabel::setAlignment(spk::HorizontalAlignment p_horizontalAlignment, spk::VerticalAlignment p_verticalAlignment)
	{
		if (_horizontalAlignment == p_horizontalAlignment && _verticalAlignment == p_verticalAlignment)
		{
			return;
		}

		_horizontalAlignment = p_horizontalAlignment;
		_verticalAlignment = p_verticalAlignment;
		invalidateRenderUnit();
	}

	void TextLabel::setPadding(const spk::Vector2Int& p_padding)
	{
		if (_padding == p_padding)
		{
			return;
		}

		_padding = p_padding;
		invalidateRenderUnit();
	}

	const std::shared_ptr<spk::Font>& TextLabel::font() const
	{
		return _font;
	}

	const spk::Font::Text& TextLabel::text() const
	{
		return _text;
	}

	const spk::Font::Size& TextLabel::textSize() const
	{
		return _textSize;
	}

	const spk::Color& TextLabel::glyphColor() const
	{
		return _glyphColor;
	}

	const spk::Color& TextLabel::outlineColor() const
	{
		return _outlineColor;
	}

	float TextLabel::depth() const
	{
		return _depth;
	}

	spk::HorizontalAlignment TextLabel::horizontalAlignment() const
	{
		return _horizontalAlignment;
	}

	spk::VerticalAlignment TextLabel::verticalAlignment() const
	{
		return _verticalAlignment;
	}

	const spk::Vector2Int& TextLabel::padding() const
	{
		return _padding;
	}
}
