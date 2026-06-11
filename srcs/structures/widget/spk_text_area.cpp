#include "structures/widget/spk_text_area.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"
#include "structures/graphics/rendering/command/spk_text_render_command.hpp"

namespace
{
	[[nodiscard]] bool sameColor(const spk::Color& p_left, const spk::Color& p_right)
	{
		return p_left.r == p_right.r &&
			p_left.g == p_right.g &&
			p_left.b == p_right.b &&
			p_left.a == p_right.a;
	}

	[[nodiscard]] std::vector<spk::Font::Text> splitParagraphs(const spk::Font::Text& p_text)
	{
		std::vector<spk::Font::Text> result;

		size_t start = 0;
		while (start <= p_text.size())
		{
			const size_t end = p_text.find(U'\n', start);
			if (end == spk::Font::Text::npos)
			{
				result.push_back(p_text.substr(start));
				break;
			}

			result.push_back(p_text.substr(start, end - start));
			start = end + 1;
		}

		return result;
	}
}

namespace spk
{
	TextArea::TextArea(const std::string& p_name, spk::Widget* p_parent) :
		TextArea(p_name, "", p_parent)
	{
	}

	TextArea::TextArea(
		const std::string& p_name,
		std::string_view p_text,
		spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_text(spk::Font::textFromUTF8(p_text))
	{
		useDefaultStyle();
		activate();
	}

	TextArea::TextArea(
		const std::string& p_name,
		std::string_view p_text,
		const spk::WidgetStyle& p_style,
		spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_text(spk::Font::textFromUTF8(p_text))
	{
		useStyle(p_style);
		activate();
	}

	void TextArea::_bindStyle(const spk::WidgetStyle& p_style)
	{
		_styleEditionContract = p_style.subscribeToEdition([this](const spk::WidgetStyle& p_editedStyle)
		{
			applyStyle(p_editedStyle);
		});
	}

	void TextArea::applyStyle(const spk::WidgetStyle& p_style)
	{
		setSpriteSheet(p_style.nineSliceSpriteSheet());
		setCornerSize(p_style.nineSliceCornerSize());
		setFont(p_style.font());
		setTextSize(p_style.textSize());
		setGlyphColor(p_style.glyphColor());
		setOutlineColor(p_style.outlineColor());
	}

	void TextArea::useDefaultStyle()
	{
		_bindStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
		applyStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
	}

	void TextArea::useStyle(const spk::WidgetStyle& p_style)
	{
		_bindStyle(p_style);
		applyStyle(p_style);
	}

	spk::Rect2D TextArea::_innerGeometry() const
	{
		return geometry().shrink(_cornerSize);
	}

	unsigned int TextArea::_lineHeight() const
	{
		if (_font == nullptr)
		{
			return 0;
		}

		return _font->computeStringSize(spk::Font::Text(U"Ajp|"), _textSize).y;
	}

	std::vector<spk::Font::Text> TextArea::_composeLines(unsigned int p_availableWidth) const
	{
		std::vector<spk::Font::Text> result;

		if (_font == nullptr)
		{
			return result;
		}

		for (const spk::Font::Text& paragraph : splitParagraphs(_text))
		{
			if (paragraph.empty() == true)
			{
				result.push_back(paragraph);
				continue;
			}

			spk::Font::Text currentLine;
			size_t wordStart = 0;

			while (wordStart < paragraph.size())
			{
				size_t wordEnd = paragraph.find(U' ', wordStart);
				if (wordEnd == spk::Font::Text::npos)
				{
					wordEnd = paragraph.size();
				}

				const spk::Font::Text word = paragraph.substr(wordStart, wordEnd - wordStart);
				const spk::Font::Text candidate =
					(currentLine.empty() == true) ? word : currentLine + U' ' + word;

				if (_font->computeStringSize(candidate, _textSize).x <= p_availableWidth ||
					currentLine.empty() == true)
				{
					currentLine = candidate;
				}
				else
				{
					result.push_back(currentLine);
					currentLine = word;
				}

				wordStart = wordEnd + 1;
			}

			if (currentLine.empty() == false)
			{
				result.push_back(currentLine);
			}
		}

		return result;
	}

	spk::RenderUnit TextArea::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (geometry().empty() == true)
		{
			return builder.build();
		}

		if (_spriteSheet != nullptr && _backgroundVisible == true)
		{
			builder.emplace<spk::NineSliceRenderCommand>(
				*_spriteSheet,
				geometry(),
				_cornerSize,
				_depth);
		}

		const spk::Rect2D inner = _innerGeometry();

		if (_font == nullptr || _text.empty() == true || inner.empty() == true)
		{
			return builder.build();
		}

		const std::vector<spk::Font::Text> lines = _composeLines(inner.width());
		const unsigned int lineHeight = _lineHeight();
		const unsigned int blockHeight =
			(lines.empty() == true)
				? 0
				: static_cast<unsigned int>(lines.size()) * lineHeight +
					static_cast<unsigned int>(lines.size() - 1) * static_cast<unsigned int>(_linePadding);

		int lineTop = inner.top();
		if (_verticalAlignment == spk::VerticalAlignment::Centered)
		{
			lineTop = inner.top() + (static_cast<int>(inner.height()) - static_cast<int>(blockHeight)) / 2;
		}
		else if (_verticalAlignment == spk::VerticalAlignment::Down)
		{
			lineTop = inner.bottom() - static_cast<int>(blockHeight);
		}

		for (const spk::Font::Text& line : lines)
		{
			if (line.empty() == false)
			{
				int anchorX = inner.left();
				if (_horizontalAlignment == spk::HorizontalAlignment::Centered)
				{
					anchorX = inner.left() + static_cast<int>(inner.width() / 2);
				}
				else if (_horizontalAlignment == spk::HorizontalAlignment::Right)
				{
					anchorX = inner.right();
				}

				builder.emplace<spk::TextRenderCommand>(
					*_font,
					line,
					_textSize,
					_glyphColor,
					_outlineColor,
					_depth,
					spk::Vector2Int(anchorX, lineTop),
					_horizontalAlignment,
					spk::VerticalAlignment::Top);
			}

			lineTop += static_cast<int>(lineHeight + _linePadding);
		}

		return builder.build();
	}

	spk::Vector2UInt TextArea::computeMinimalSize(unsigned int p_availableWidth) const
	{
		if (_font == nullptr)
		{
			return {0, 0};
		}

		const unsigned int innerWidth =
			(p_availableWidth > static_cast<unsigned int>(_cornerSize.x * 2))
				? p_availableWidth - static_cast<unsigned int>(_cornerSize.x * 2)
				: 0;

		const std::vector<spk::Font::Text> lines = _composeLines(innerWidth);

		unsigned int maxWidth = 0;
		for (const spk::Font::Text& line : lines)
		{
			maxWidth = std::max(maxWidth, _font->computeStringSize(line, _textSize).x);
		}

		const unsigned int blockHeight =
			(lines.empty() == true)
				? 0
				: static_cast<unsigned int>(lines.size()) * _lineHeight() +
					static_cast<unsigned int>(lines.size() - 1) * static_cast<unsigned int>(_linePadding);

		return {
			maxWidth + static_cast<unsigned int>(_cornerSize.x * 2),
			blockHeight + static_cast<unsigned int>(_cornerSize.y * 2)};
	}

	void TextArea::setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("TextArea sprite sheet cannot be null");
		}

		if (p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 3})
		{
			throw std::invalid_argument("TextArea sprite sheet must contain a 3x3 grid");
		}

		_spriteSheet = std::move(p_spriteSheet);
		invalidateRenderUnit();
	}

	void TextArea::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		if (p_cornerSize.x < 0 || p_cornerSize.y < 0)
		{
			throw std::invalid_argument("TextArea corner size cannot be negative");
		}

		if (_cornerSize == p_cornerSize)
		{
			return;
		}

		_cornerSize = p_cornerSize;
		invalidateRenderUnit();
	}

	void TextArea::setFont(std::shared_ptr<spk::Font> p_font)
	{
		if (p_font == nullptr)
		{
			throw std::invalid_argument("TextArea font cannot be null");
		}

		_font = std::move(p_font);
		invalidateRenderUnit();
	}

	void TextArea::setTextSize(const spk::Font::Size& p_textSize)
	{
		if (_textSize == p_textSize)
		{
			return;
		}

		_textSize = p_textSize;
		invalidateRenderUnit();
	}

	void TextArea::setGlyphColor(const spk::Color& p_color)
	{
		if (sameColor(_glyphColor, p_color) == true)
		{
			return;
		}

		_glyphColor = p_color;
		invalidateRenderUnit();
	}

	void TextArea::setOutlineColor(const spk::Color& p_color)
	{
		if (sameColor(_outlineColor, p_color) == true)
		{
			return;
		}

		_outlineColor = p_color;
		invalidateRenderUnit();
	}

	void TextArea::setDepth(float p_depth)
	{
		if (_depth == p_depth)
		{
			return;
		}

		_depth = p_depth;
		invalidateRenderUnit();
	}

	void TextArea::setText(const spk::Font::Text& p_text)
	{
		if (_text == p_text)
		{
			return;
		}

		_text = p_text;
		invalidateRenderUnit();
	}

	void TextArea::setText(std::string_view p_text)
	{
		setText(spk::Font::textFromUTF8(p_text));
	}

	void TextArea::setBackgroundVisible(bool p_state)
	{
		if (_backgroundVisible == p_state)
		{
			return;
		}

		_backgroundVisible = p_state;
		invalidateRenderUnit();
	}

	bool TextArea::isBackgroundVisible() const
	{
		return _backgroundVisible;
	}

	void TextArea::setLinePadding(size_t p_linePadding)
	{
		if (_linePadding == p_linePadding)
		{
			return;
		}

		_linePadding = p_linePadding;
		invalidateRenderUnit();
	}

	void TextArea::setAlignment(
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment)
	{
		if (_horizontalAlignment == p_horizontalAlignment && _verticalAlignment == p_verticalAlignment)
		{
			return;
		}

		_horizontalAlignment = p_horizontalAlignment;
		_verticalAlignment = p_verticalAlignment;
		invalidateRenderUnit();
	}

	const spk::Font::Text& TextArea::text() const
	{
		return _text;
	}

	size_t TextArea::linePadding() const
	{
		return _linePadding;
	}

	spk::HorizontalAlignment TextArea::horizontalAlignment() const
	{
		return _horizontalAlignment;
	}

	spk::VerticalAlignment TextArea::verticalAlignment() const
	{
		return _verticalAlignment;
	}

	const std::shared_ptr<spk::SpriteSheet>& TextArea::spriteSheet() const
	{
		return _spriteSheet;
	}

	const spk::Vector2Int& TextArea::cornerSize() const
	{
		return _cornerSize;
	}

	const std::shared_ptr<spk::Font>& TextArea::font() const
	{
		return _font;
	}

	const spk::Font::Size& TextArea::textSize() const
	{
		return _textSize;
	}

	const spk::Color& TextArea::glyphColor() const
	{
		return _glyphColor;
	}

	const spk::Color& TextArea::outlineColor() const
	{
		return _outlineColor;
	}

	float TextArea::depth() const
	{
		return _depth;
	}
}
