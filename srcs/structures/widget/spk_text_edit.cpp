#include "structures/widget/spk_text_edit.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/command/spk_color_rectangle_render_command.hpp"
#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"
#include "structures/graphics/rendering/command/spk_text_render_command.hpp"

namespace
{
	[[nodiscard]] bool sameColor(const spk::Color &p_left, const spk::Color &p_right)
	{
		return p_left.r == p_right.r &&
			   p_left.g == p_right.g &&
			   p_left.b == p_right.b &&
			   p_left.a == p_right.a;
	}

	[[nodiscard]] std::string toUTF8(const spk::Font::Text &p_text)
	{
		std::string result;
		result.reserve(p_text.size());

		for (char32_t codepoint : p_text)
		{
			if (codepoint <= 0x7F)
			{
				result.push_back(static_cast<char>(codepoint));
			}
			else if (codepoint <= 0x7FF)
			{
				result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
				result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
			}
			else if (codepoint <= 0xFFFF)
			{
				result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
				result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
			}
			else
			{
				result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
				result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
			}
		}

		return result;
	}
}

namespace spk
{
	void TextEdit::_configureSizeCache()
	{
		configureMinimalSizeGenerator([this]() {
			spk::Vector2UInt result = {0, 0};

			if (_font != nullptr && _placeholder.empty() == false)
			{
				const spk::Vector2UInt placeholderSize = _font->computeStringSize(_placeholder, _textSize);
				result.x = placeholderSize.x + static_cast<unsigned int>(_cornerSize.x * 2);
				result.y = placeholderSize.y + static_cast<unsigned int>(_cornerSize.y * 2);
			}

			return result;
		});
	}

	TextEdit::TextEdit(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_configureSizeCache();
		useDefaultStyle();
		activate();
	}

	TextEdit::TextEdit(
		const std::string &p_name,
		const spk::WidgetStyle &p_style,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_configureSizeCache();
		useStyle(p_style);
		activate();
	}

	void TextEdit::_bindStyle(const spk::WidgetStyle &p_style)
	{
		_styleEditionContract = p_style.subscribeToEdition([this](const spk::WidgetStyle &p_editedStyle) {
			applyStyle(p_editedStyle);
		});
	}

	void TextEdit::applyStyle(const spk::WidgetStyle &p_style)
	{
		setSpriteSheet(p_style.nineSliceSpriteSheet());
		setCornerSize(p_style.nineSliceCornerSize());
		setFont(p_style.font());
		setTextSize(p_style.textSize());
		setGlyphColor(p_style.glyphColor());
		setOutlineColor(p_style.outlineColor());
	}

	void TextEdit::useDefaultStyle()
	{
		_bindStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
		applyStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
	}

	void TextEdit::useStyle(const spk::WidgetStyle &p_style)
	{
		_bindStyle(p_style);
		applyStyle(p_style);
	}

	spk::Rect2D TextEdit::_innerGeometry() const
	{
		return geometry().shrink(_cornerSize);
	}

	void TextEdit::_computeCursorsValues() const
	{
		if (_font == nullptr)
		{
			return;
		}

		const spk::Font::Text textToRender = renderedText();
		const unsigned int availableWidth = _innerGeometry().width();

		if (_needLowerCursorUpdate == true)
		{
			_lowerCursor = _cursor;

			while (_lowerCursor != 0 &&
				   _font->computeStringSize(
							textToRender.substr(_lowerCursor - 1, _cursor - _lowerCursor + 1), _textSize)
						   .x <= availableWidth)
			{
				_lowerCursor--;
			}

			_needLowerCursorUpdate = false;
		}

		if (_needHigherCursorUpdate == true)
		{
			_higherCursor = _cursor;

			while (_higherCursor < textToRender.size() &&
				   _font->computeStringSize(
							textToRender.substr(_lowerCursor, _higherCursor - _lowerCursor + 1), _textSize)
						   .x <= availableWidth)
			{
				_higherCursor++;
			}

			_needHigherCursorUpdate = false;
		}
	}

	spk::RenderUnit TextEdit::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (geometry().empty() == true)
		{
			return builder.build();
		}

		if (_spriteSheet != nullptr)
		{
			builder.emplace<spk::NineSliceRenderCommand>(
				*_spriteSheet,
				geometry(),
				_cornerSize,
				_depth);
		}

		_computeCursorsValues();

		const spk::Rect2D inner = _innerGeometry();
		const spk::Font::Text textToRender = renderedText();
		const spk::Font::Text visibleText = textToRender.substr(_lowerCursor, _higherCursor - _lowerCursor);

		if (_font != nullptr && visibleText.empty() == false)
		{
			const spk::Vector2Int textAnchor = {
				inner.left(),
				inner.top() + static_cast<int>(inner.height() / 2)};

			builder.emplace<spk::TextRenderCommand>(
				_font,
				visibleText,
				_textSize,
				_glyphColor,
				_outlineColor,
				_depth,
				textAnchor,
				spk::HorizontalAlignment::Left,
				spk::VerticalAlignment::Centered);
		}

		if (_font != nullptr &&
			_isEditEnabled == true &&
			_renderCursor == true &&
			hasFocus(FocusType::Keyboard) == true)
		{
			const size_t visibleCursor = (_cursor >= _lowerCursor) ? _cursor - _lowerCursor : 0;
			const unsigned int textWidthBeforeCursor =
				_font->computeStringSize(visibleText.substr(0, visibleCursor), _textSize).x;

			builder.emplace<spk::ColorRectangleRenderCommand>(
				spk::Rect2D(
					inner.left() + static_cast<int>(textWidthBeforeCursor),
					inner.top(),
					2,
					inner.height()),
				_cursorColor,
				_depth);
		}

		return builder.build();
	}

	void TextEdit::_onGeometryChange()
	{
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
	}

	void TextEdit::_onUpdate(const spk::UpdateContext &p_tick)
	{
		const bool newRenderCursor = (p_tick.timestamp.milliseconds() / 250) % 2 == 0;

		if (newRenderCursor != _renderCursor)
		{
			_renderCursor = newRenderCursor;
			if (hasFocus(FocusType::Keyboard) == true)
			{
				invalidateRenderUnit();
			}
		}
	}

	void TextEdit::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		const bool hovered = absoluteGeometry().contains(p_event.device().position);
		if (hovered == _isHovered)
		{
			return;
		}

		_isHovered = hovered;

		if (_isEditEnabled == false)
		{
			return;
		}

		p_event.device().requestCursor(hovered == true ? "IBeam" : "Arrow");
	}

	void TextEdit::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (_isEditEnabled == false)
		{
			releaseFocus(FocusType::Keyboard);
			return;
		}

		if (p_event->button != spk::Mouse::Left)
		{
			return;
		}

		if (absoluteGeometry().contains(p_event.device().position) == true)
		{
			takeFocus(FocusType::Keyboard);
			invalidateRenderUnit();
			p_event.consume();
		}
		else if (hasFocus(FocusType::Keyboard) == true)
		{
			releaseFocus(FocusType::Keyboard);
			invalidateRenderUnit();
		}
	}

	void TextEdit::_onKeyPressedEvent(spk::KeyPressedEvent &p_event)
	{
		if (_isEditEnabled == false || hasFocus(FocusType::Keyboard) == false)
		{
			return;
		}

		switch (p_event->key)
		{
		case spk::Keyboard::LeftArrow:
			_moveCursorLeft();
			break;
		case spk::Keyboard::RightArrow:
			_moveCursorRight();
			break;
		case spk::Keyboard::Delete:
			_deleteAtCursor();
			break;
		case spk::Keyboard::Backspace:
			_backspace();
			break;
		case spk::Keyboard::Escape:
			releaseFocus(FocusType::Keyboard);
			invalidateRenderUnit();
			break;
		default:
			return;
		}

		p_event.consume();
	}

	void TextEdit::_onTextInputEvent(spk::TextInputEvent &p_event)
	{
		if (_isEditEnabled == false || hasFocus(FocusType::Keyboard) == false)
		{
			return;
		}

		if (p_event->glyph < U' ')
		{
			return;
		}

		_insertGlyph(p_event->glyph);
		p_event.consume();
	}

	void TextEdit::_moveCursorLeft()
	{
		if (_cursor == 0)
		{
			return;
		}

		_cursor--;
		if (_cursor < _lowerCursor)
		{
			_lowerCursor = _cursor;
			_needHigherCursorUpdate = true;
		}
		invalidateRenderUnit();
	}

	void TextEdit::_moveCursorRight()
	{
		if (_cursor >= _text.size())
		{
			return;
		}

		_cursor++;
		if (_cursor >= _higherCursor)
		{
			_higherCursor = _cursor;
			_needLowerCursorUpdate = true;
		}
		invalidateRenderUnit();
	}

	void TextEdit::_notifyEdition()
	{
		_onEditionProvider.trigger(_text);
	}

	TextEdit::ValidationState TextEdit::_validate(const spk::Font::Text &p_text) const
	{
		if (_validationCallback == nullptr)
		{
			return ValidationState::Valid;
		}

		return _validationCallback(p_text);
	}

	TextEdit::EditionContract TextEdit::subscribeToEdition(EditionCallback p_callback)
	{
		return _onEditionProvider.subscribe(std::move(p_callback));
	}

	void TextEdit::setValidationCallback(ValidationCallback p_callback)
	{
		_validationCallback = std::move(p_callback);
	}

	TextEdit::ValidationState TextEdit::validationState() const
	{
		return _validate(_text);
	}

	void TextEdit::_deleteAtCursor()
	{
		if (_cursor >= _text.size())
		{
			return;
		}

		_text.erase(_cursor, 1);
		_needHigherCursorUpdate = true;
		invalidateRenderUnit();
		_notifyEdition();
	}

	void TextEdit::_backspace()
	{
		if (_text.empty() == true || _cursor == 0)
		{
			return;
		}

		_text.erase(_cursor - 1, 1);
		_cursor--;

		if (_cursor <= _lowerCursor)
		{
			_lowerCursor = (_lowerCursor > 0 && _cursor > 0) ? _cursor - 1 : 0;
			_needHigherCursorUpdate = true;
		}
		invalidateRenderUnit();
		_notifyEdition();
	}

	void TextEdit::_insertGlyph(char32_t p_glyph)
	{
		spk::Font::Text candidate = _text;
		candidate.insert(_cursor, 1, p_glyph);

		if (_validate(candidate) == ValidationState::Invalid)
		{
			return;
		}

		_text = std::move(candidate);
		_cursor++;

		if (_cursor > _higherCursor)
		{
			_higherCursor = _cursor;
			_needLowerCursorUpdate = true;
			_needHigherCursorUpdate = true;
		}
		invalidateRenderUnit();
		_notifyEdition();
	}

	void TextEdit::setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("TextEdit sprite sheet cannot be null");
		}

		if (p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 3})
		{
			throw std::invalid_argument("TextEdit sprite sheet must contain a 3x3 grid");
		}

		_spriteSheet = std::move(p_spriteSheet);
		invalidateRenderUnit();
	}

	void TextEdit::setCornerSize(const spk::Vector2Int &p_cornerSize)
	{
		if (p_cornerSize.x < 0 || p_cornerSize.y < 0)
		{
			throw std::invalid_argument("TextEdit corner size cannot be negative");
		}

		if (_cornerSize == p_cornerSize)
		{
			return;
		}

		_cornerSize = p_cornerSize;
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
		invalidateRenderUnit();
	}

	void TextEdit::setFont(std::shared_ptr<spk::Font> p_font)
	{
		if (p_font == nullptr)
		{
			throw std::invalid_argument("TextEdit font cannot be null");
		}

		_font = std::move(p_font);
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
		invalidateRenderUnit();
	}

	void TextEdit::setTextSize(const spk::Font::Size &p_textSize)
	{
		if (_textSize == p_textSize)
		{
			return;
		}

		_textSize = p_textSize;
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
		invalidateRenderUnit();
	}

	void TextEdit::setGlyphColor(const spk::Color &p_color)
	{
		if (sameColor(_glyphColor, p_color) == true)
		{
			return;
		}

		_glyphColor = p_color;
		invalidateRenderUnit();
	}

	void TextEdit::setOutlineColor(const spk::Color &p_color)
	{
		if (sameColor(_outlineColor, p_color) == true)
		{
			return;
		}

		_outlineColor = p_color;
		invalidateRenderUnit();
	}

	void TextEdit::setCursorColor(const spk::Color &p_color)
	{
		if (sameColor(_cursorColor, p_color) == true)
		{
			return;
		}

		_cursorColor = p_color;
		invalidateRenderUnit();
	}

	void TextEdit::setDepth(float p_depth)
	{
		if (_depth == p_depth)
		{
			return;
		}

		_depth = p_depth;
		invalidateRenderUnit();
	}

	void TextEdit::setText(const spk::Font::Text &p_text)
	{
		_text = p_text;
		_cursor = _text.size();
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
		invalidateRenderUnit();
		_notifyEdition();
	}

	void TextEdit::setText(std::string_view p_text)
	{
		setText(spk::Font::textFromUTF8(p_text));
	}

	void TextEdit::setPlaceholder(const spk::Font::Text &p_placeholder)
	{
		_placeholder = p_placeholder;
		if (_text.empty() == true)
		{
			_needLowerCursorUpdate = true;
			_needHigherCursorUpdate = true;
			invalidateRenderUnit();
		}
	}

	void TextEdit::setPlaceholder(std::string_view p_placeholder)
	{
		setPlaceholder(spk::Font::textFromUTF8(p_placeholder));
	}

	void TextEdit::setObscured(bool p_state)
	{
		if (_isObscured == p_state)
		{
			return;
		}

		_isObscured = p_state;
		_needLowerCursorUpdate = true;
		_needHigherCursorUpdate = true;
		invalidateRenderUnit();
	}

	void TextEdit::enableEdit()
	{
		_isEditEnabled = true;
	}

	void TextEdit::disableEdit()
	{
		_isEditEnabled = false;
		releaseFocus(FocusType::Keyboard);
		invalidateRenderUnit();
	}

	bool TextEdit::hasText() const
	{
		return _text.empty() == false;
	}

	bool TextEdit::isObscured() const
	{
		return _isObscured;
	}

	bool TextEdit::isEditEnabled() const
	{
		return _isEditEnabled;
	}

	spk::Font::Text TextEdit::renderedText() const
	{
		if (hasText() == true)
		{
			if (_isObscured == true)
			{
				return spk::Font::Text(_text.size(), U'*');
			}
			return _text;
		}
		return _placeholder;
	}

	const spk::Font::Text &TextEdit::text() const
	{
		return _text;
	}

	std::string TextEdit::textAsUTF8() const
	{
		return toUTF8(_text);
	}

	const spk::Font::Text &TextEdit::placeholder() const
	{
		return _placeholder;
	}

	size_t TextEdit::cursor() const
	{
		return _cursor;
	}

	const std::shared_ptr<spk::SpriteSheet> &TextEdit::spriteSheet() const
	{
		return _spriteSheet;
	}

	const spk::Vector2Int &TextEdit::cornerSize() const
	{
		return _cornerSize;
	}

	const std::shared_ptr<spk::Font> &TextEdit::font() const
	{
		return _font;
	}

	const spk::Font::Size &TextEdit::textSize() const
	{
		return _textSize;
	}

	const spk::Color &TextEdit::glyphColor() const
	{
		return _glyphColor;
	}

	const spk::Color &TextEdit::outlineColor() const
	{
		return _outlineColor;
	}

	float TextEdit::depth() const
	{
		return _depth;
	}
}
